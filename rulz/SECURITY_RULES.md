# VyroEngine Security Rules

**Version:** 1.0.0

---

## Asset Security

### Rule SEC-1: Validate All Loaded Assets
Every asset loaded from disk must be validated before use.

```cpp
std::expected<Texture, Error> load_texture(std::string_view path) {
    auto data = read_file(path);
    if (!data) return std::unexpected(Error::FileNotFound);

    // Validate header before passing to stb_image
    if (!is_valid_image_header(data->bytes)) {
        VYRO_ERROR(ASSET, "Invalid image header: {}", path);
        return std::unexpected(Error::InvalidFormat);
    }

    return decode_image(*data);
}
```

### Rule SEC-2: Path Traversal Prevention
Never allow asset paths containing `..` to escape the project directory.

```cpp
bool is_safe_path(std::string_view path, std::string_view root) {
    auto canonical = std::filesystem::weakly_canonical(root / path);
    return canonical.string().starts_with(root);
}
```

### Rule SEC-3: Asset Size Limits
Every asset loader enforces size limits before decompression to prevent decompression bombs.

| Asset Type | Max Size |
|-----------|---------|
| Texture | 8192×8192 |
| Audio clip (uncompressed) | 256 MB |
| Mesh | 10M vertices |
| Script | 1 MB source |
| Scene | 512 MB |

---

## Script Security

### Rule SEC-4: Lua Sandbox
Scripts run in a restricted sandbox. File I/O, OS access, and dynamic loading are disabled. (See `SCRIPTING_RULES.md` Rule SEC-LUA-1.)

### Rule SEC-5: No eval() Equivalent
No engine feature accepts arbitrary code strings from user input or network packets and executes them.

---

## Networking Security

### Rule SEC-6: Validate All Incoming Packets
Every received network packet is validated before processing:
- Packet size bounds check
- Packet type validation against known enum values
- Payload deserialization with bounds checking
- Rate limiting per connection

```cpp
bool NetworkManager::process_packet(const Packet& p) {
    if (p.size > MAX_PACKET_SIZE) {
        disconnect(p.sender, DisconnectReason::PacketTooLarge);
        return false;
    }
    if (!is_valid_packet_type(p.type)) {
        disconnect(p.sender, DisconnectReason::InvalidPacketType);
        return false;
    }
    return true;
}
```

### Rule SEC-7: Server is Authoritative
The server never trusts client-reported game state. Clients send inputs; the server simulates and validates outcomes.

### Rule SEC-8: No Client-Side Cheat Authority
Position, health, damage, collision — all validated server-side. Client predictions are suggestions, not facts.

---

## Build Security

### Rule SEC-9: Debug Info Stripped in Release
Release builds strip debug symbols, assert strings, and internal logging. Do not ship PDB/DWARF in production packages.

### Rule SEC-10: No Hardcoded Secrets
No API keys, passwords, or cryptographic keys in source code. Use environment variables or config files excluded from git.

```
# .gitignore
*.secret
config/secrets.json
.env
```

### Rule SEC-11: Dependency Audits
Third-party dependencies are pinned to exact versions in `vcpkg.json`. Security advisories are reviewed monthly.
