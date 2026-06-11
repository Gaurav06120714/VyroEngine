// VyroEngine — Scripting system implementation
// A recursive-descent interpreter over a token stream. Statements execute
// against the engine's global table; host functions bridge into C++.
#include "vyro/scripting/ScriptEngine.hpp"

#include <cctype>
#include <cmath>

namespace vyro {

namespace {

// ── Tokenizer ────────────────────────────────────────────────────────
enum class Tok : u8 {
    Number, Ident, Plus, Minus, Star, Slash,
    Assign, Eq, Ne, Lt, Gt, Le, Ge,
    LParen, RParen, LBrace, RBrace, Comma,
    If, Else, While, Newline, End,
};

struct Token {
    Tok kind = Tok::End;
    f64 number = 0.0;
    std::string text;
};

class Lexer
{
public:
    explicit Lexer(std::string_view src) : m_src(src) {}

    std::vector<Token> tokenize(bool& ok)
    {
        std::vector<Token> out;
        ok = true;
        while (m_pos < m_src.size()) {
            const char c = m_src[m_pos];
            if (c == '#') { // comment to end of line
                while (m_pos < m_src.size() && m_src[m_pos] != '\n') {
                    ++m_pos;
                }
            } else if (c == '\n') {
                out.push_back(Token{Tok::Newline, 0.0, {}});
                ++m_pos;
            } else if (std::isspace(static_cast<unsigned char>(c)) != 0) {
                ++m_pos;
            } else if (std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '.') {
                usize len = 0;
                const f64 value = std::stod(std::string(m_src.substr(m_pos)), &len);
                out.push_back(Token{Tok::Number, value, {}});
                m_pos += len;
            } else if (std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_') {
                usize end = m_pos;
                while (end < m_src.size()
                       && (std::isalnum(static_cast<unsigned char>(m_src[end])) != 0 || m_src[end] == '_')) {
                    ++end;
                }
                const std::string word(m_src.substr(m_pos, end - m_pos));
                Token t;
                t.text = word;
                t.kind = word == "if" ? Tok::If : word == "else" ? Tok::Else
                       : word == "while" ? Tok::While : Tok::Ident;
                out.push_back(t);
                m_pos = end;
            } else {
                if (!punct(out)) {
                    ok = false;
                    return out;
                }
            }
        }
        out.push_back(Token{Tok::End, 0.0, {}});
        return out;
    }

private:
    bool punct(std::vector<Token>& out)
    {
        const char c = m_src[m_pos];
        const char n = (m_pos + 1 < m_src.size()) ? m_src[m_pos + 1] : '\0';
        auto two = [&](Tok k) { out.push_back(Token{k, 0.0, {}}); m_pos += 2; return true; };
        auto one = [&](Tok k) { out.push_back(Token{k, 0.0, {}}); ++m_pos; return true; };
        switch (c) {
            case '+': return one(Tok::Plus);
            case '-': return one(Tok::Minus);
            case '*': return one(Tok::Star);
            case '/': return one(Tok::Slash);
            case '(': return one(Tok::LParen);
            case ')': return one(Tok::RParen);
            case '{': return one(Tok::LBrace);
            case '}': return one(Tok::RBrace);
            case ',': return one(Tok::Comma);
            case '=': return n == '=' ? two(Tok::Eq) : one(Tok::Assign);
            case '!': return n == '=' ? two(Tok::Ne) : false;
            case '<': return n == '=' ? two(Tok::Le) : one(Tok::Lt);
            case '>': return n == '=' ? two(Tok::Ge) : one(Tok::Gt);
            default: return false;
        }
    }

    std::string_view m_src;
    usize m_pos = 0;
};

// ── Parser / evaluator ───────────────────────────────────────────────
class Interp
{
public:
    Interp(const std::vector<Token>& toks,
           std::unordered_map<std::string, f64>& globals,
           const std::unordered_map<std::string, ScriptEngine::HostFunction>& functions)
        : m_toks(toks), m_globals(globals), m_functions(functions)
    {
    }

    std::expected<void, ScriptError> run_block(bool execute)
    {
        while (true) {
            skip_newlines();
            const Tok k = peek().kind;
            if (k == Tok::End || k == Tok::RBrace) {
                return {};
            }
            if (auto r = statement(execute); !r.has_value()) {
                return std::unexpected(r.error());
            }
        }
    }

    std::expected<f64, ScriptError> expression()
    {
        return comparison();
    }

private:
    const Token& peek() const { return m_toks[m_pos]; }
    const Token& advance() { return m_toks[m_pos++]; }
    bool match(Tok k)
    {
        if (peek().kind == k) {
            ++m_pos;
            return true;
        }
        return false;
    }
    void skip_newlines()
    {
        while (peek().kind == Tok::Newline) {
            ++m_pos;
        }
    }

    std::expected<void, ScriptError> statement(bool execute)
    {
        if (peek().kind == Tok::If) {
            return if_statement(execute);
        }
        if (peek().kind == Tok::While) {
            return while_statement(execute);
        }
        if (peek().kind == Tok::Ident) {
            // assignment: ident '=' expr   |   bare call: ident '(' args ')'
            const std::string name = peek().text;
            if (m_toks[m_pos + 1].kind == Tok::Assign) {
                m_pos += 2;
                auto value = expression();
                if (!value.has_value()) {
                    return std::unexpected(value.error());
                }
                if (execute) {
                    m_globals[name] = value.value();
                }
                return {};
            }
        }
        auto value = expression(); // expression statement (e.g. host call)
        if (!value.has_value()) {
            return std::unexpected(value.error());
        }
        (void)execute;
        return {};
    }

    std::expected<void, ScriptError> if_statement(bool execute)
    {
        advance(); // if
        auto cond = expression();
        if (!cond.has_value()) {
            return std::unexpected(cond.error());
        }
        if (!match(Tok::LBrace)) {
            return std::unexpected(ScriptError::SyntaxError);
        }
        const bool take = execute && cond.value() != 0.0;
        if (auto r = run_block(take); !r.has_value()) {
            return r;
        }
        if (!match(Tok::RBrace)) {
            return std::unexpected(ScriptError::SyntaxError);
        }
        skip_newlines();
        if (peek().kind == Tok::Else) {
            advance();
            skip_newlines();
            if (!match(Tok::LBrace)) {
                return std::unexpected(ScriptError::SyntaxError);
            }
            if (auto r = run_block(execute && cond.value() == 0.0); !r.has_value()) {
                return r;
            }
            if (!match(Tok::RBrace)) {
                return std::unexpected(ScriptError::SyntaxError);
            }
        }
        return {};
    }

    std::expected<void, ScriptError> while_statement(bool execute)
    {
        advance(); // while
        const usize cond_pos = m_pos;
        constexpr int kMaxIterations = 1'000'000; // runaway-loop guard
        int iterations = 0;
        while (true) {
            m_pos = cond_pos;
            auto cond = expression();
            if (!cond.has_value()) {
                return std::unexpected(cond.error());
            }
            if (!match(Tok::LBrace)) {
                return std::unexpected(ScriptError::SyntaxError);
            }
            const bool take = execute && cond.value() != 0.0 && iterations < kMaxIterations;
            if (auto r = run_block(take); !r.has_value()) {
                return r;
            }
            if (!match(Tok::RBrace)) {
                return std::unexpected(ScriptError::SyntaxError);
            }
            if (!take) {
                return {};
            }
            ++iterations;
        }
    }

    std::expected<f64, ScriptError> comparison()
    {
        auto left = additive();
        if (!left.has_value()) {
            return left;
        }
        while (true) {
            const Tok k = peek().kind;
            if (k != Tok::Eq && k != Tok::Ne && k != Tok::Lt && k != Tok::Gt && k != Tok::Le && k != Tok::Ge) {
                return left;
            }
            advance();
            auto right = additive();
            if (!right.has_value()) {
                return right;
            }
            const f64 a = left.value();
            const f64 b = right.value();
            switch (k) {
                case Tok::Eq: left = (a == b) ? 1.0 : 0.0; break;
                case Tok::Ne: left = (a != b) ? 1.0 : 0.0; break;
                case Tok::Lt: left = (a < b) ? 1.0 : 0.0; break;
                case Tok::Gt: left = (a > b) ? 1.0 : 0.0; break;
                case Tok::Le: left = (a <= b) ? 1.0 : 0.0; break;
                default: left = (a >= b) ? 1.0 : 0.0; break;
            }
        }
    }

    std::expected<f64, ScriptError> additive()
    {
        auto left = multiplicative();
        if (!left.has_value()) {
            return left;
        }
        while (peek().kind == Tok::Plus || peek().kind == Tok::Minus) {
            const Tok k = advance().kind;
            auto right = multiplicative();
            if (!right.has_value()) {
                return right;
            }
            left = (k == Tok::Plus) ? left.value() + right.value() : left.value() - right.value();
        }
        return left;
    }

    std::expected<f64, ScriptError> multiplicative()
    {
        auto left = unary();
        if (!left.has_value()) {
            return left;
        }
        while (peek().kind == Tok::Star || peek().kind == Tok::Slash) {
            const Tok k = advance().kind;
            auto right = unary();
            if (!right.has_value()) {
                return right;
            }
            if (k == Tok::Slash) {
                if (right.value() == 0.0) {
                    return std::unexpected(ScriptError::DivisionByZero);
                }
                left = left.value() / right.value();
            } else {
                left = left.value() * right.value();
            }
        }
        return left;
    }

    std::expected<f64, ScriptError> unary()
    {
        if (match(Tok::Minus)) {
            auto v = unary();
            if (!v.has_value()) {
                return v;
            }
            return -v.value();
        }
        return primary();
    }

    std::expected<f64, ScriptError> primary()
    {
        if (peek().kind == Tok::Number) {
            return advance().number;
        }
        if (match(Tok::LParen)) {
            auto v = expression();
            if (!v.has_value()) {
                return v;
            }
            if (!match(Tok::RParen)) {
                return std::unexpected(ScriptError::SyntaxError);
            }
            return v;
        }
        if (peek().kind == Tok::Ident) {
            const std::string name = advance().text;
            if (match(Tok::LParen)) { // host function call
                std::vector<f64> args;
                if (peek().kind != Tok::RParen) {
                    while (true) {
                        auto a = expression();
                        if (!a.has_value()) {
                            return a;
                        }
                        args.push_back(a.value());
                        if (!match(Tok::Comma)) {
                            break;
                        }
                    }
                }
                if (!match(Tok::RParen)) {
                    return std::unexpected(ScriptError::SyntaxError);
                }
                const auto fn = m_functions.find(name);
                if (fn == m_functions.end()) {
                    return std::unexpected(ScriptError::UnknownIdentifier);
                }
                return fn->second(args);
            }
            const auto it = m_globals.find(name);
            if (it == m_globals.end()) {
                return std::unexpected(ScriptError::UnknownIdentifier);
            }
            return it->second;
        }
        return std::unexpected(ScriptError::SyntaxError);
    }

    const std::vector<Token>& m_toks;
    usize m_pos = 0;
    std::unordered_map<std::string, f64>& m_globals;
    const std::unordered_map<std::string, ScriptEngine::HostFunction>& m_functions;
};

} // namespace

void ScriptEngine::register_function(std::string_view name, HostFunction fn)
{
    m_functions[std::string(name)] = std::move(fn);
}

std::expected<void, ScriptError> ScriptEngine::run(std::string_view source)
{
    bool ok = true;
    const std::vector<Token> toks = Lexer(source).tokenize(ok);
    if (!ok) {
        return std::unexpected(ScriptError::SyntaxError);
    }
    Interp interp(toks, m_globals, m_functions);
    return interp.run_block(true);
}

std::expected<f64, ScriptError> ScriptEngine::eval(std::string_view expression)
{
    bool ok = true;
    const std::vector<Token> toks = Lexer(expression).tokenize(ok);
    if (!ok) {
        return std::unexpected(ScriptError::SyntaxError);
    }
    Interp interp(toks, m_globals, m_functions);
    return interp.expression();
}

f64 ScriptEngine::get(std::string_view name) const
{
    const auto it = m_globals.find(std::string(name));
    return it != m_globals.end() ? it->second : 0.0;
}

void ScriptEngine::set(std::string_view name, f64 value)
{
    m_globals[std::string(name)] = value;
}

bool ScriptEngine::has(std::string_view name) const
{
    return m_globals.contains(std::string(name));
}

void ScriptEngine::reset()
{
    m_globals.clear();
}

} // namespace vyro
