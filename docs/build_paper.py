#!/usr/bin/env python3
"""Generate the VyroEngine research paper PDF (~20 pages)."""

from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_JUSTIFY, TA_CENTER, TA_LEFT
from reportlab.platypus import (
    BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, PageBreak,
    Table, TableStyle, ListFlowable, ListItem, HRFlowable, KeepTogether
)
from reportlab.platypus.tableofcontents import TableOfContents

OUT = "../VyroEngine_Research_Paper.pdf"

# ---------------------------------------------------------------- styles
# Pure black-and-white academic style (serif), matching the reference report.
BLACK = colors.black

styles = getSampleStyleSheet()

def S(name, **kw):
    base = kw.pop("parent", styles["Normal"])
    return ParagraphStyle(name, parent=base, **kw)

st_title = S("VTitle", fontName="Times-Bold", fontSize=24, leading=29,
             textColor=BLACK, alignment=TA_CENTER, spaceAfter=8)
st_subtitle = S("VSub", fontName="Times-Roman", fontSize=13, leading=18,
                textColor=BLACK, alignment=TA_CENTER, spaceAfter=6)
st_author = S("VAuth", fontName="Times-Roman", fontSize=12, leading=18,
              textColor=BLACK, alignment=TA_CENTER)
st_h1 = S("VH1", fontName="Times-Bold", fontSize=16, leading=20,
          textColor=BLACK, spaceBefore=16, spaceAfter=6)
st_h2 = S("VH2", fontName="Times-Bold", fontSize=12.5, leading=16,
          textColor=BLACK, spaceBefore=10, spaceAfter=5)
st_body = S("VBody", fontName="Times-Roman", fontSize=11, leading=15.5,
            textColor=BLACK, alignment=TA_JUSTIFY, spaceAfter=7)
st_abstract = S("VAbs", fontName="Times-Roman", fontSize=11, leading=15.5,
                textColor=BLACK, alignment=TA_JUSTIFY, spaceAfter=6)
st_cap = S("VCap", fontName="Times-Italic", fontSize=9, leading=12, textColor=BLACK,
           alignment=TA_CENTER, spaceBefore=3, spaceAfter=10)
st_bullet = S("VBul", fontName="Times-Roman", fontSize=11, leading=15,
              textColor=BLACK, alignment=TA_JUSTIFY)
st_code = S("VCode", fontName="Courier", fontSize=8.5, leading=11,
            textColor=BLACK, leftIndent=6, rightIndent=6,
            spaceBefore=4, spaceAfter=8, borderPadding=6,
            borderWidth=0.5, borderColor=BLACK)
st_ref = S("VRef", fontName="Times-Roman", fontSize=10, leading=13.5,
           textColor=BLACK, alignment=TA_LEFT, leftIndent=14,
           firstLineIndent=-14, spaceAfter=4)


class DocTemplate(BaseDocTemplate):
    def __init__(self, fn, **kw):
        self._cover = kw.pop("cover_page", 1)
        super().__init__(fn, **kw)
        frame = Frame(22*mm, 18*mm, A4[0]-44*mm, A4[1]-34*mm, id="main")
        self.addPageTemplates([PageTemplate(id="all", frames=[frame],
                                            onPage=self._decor)])

    def _decor(self, canvas, doc):
        # No colors, no header bars. Simple centered page number, skip cover.
        if doc.page <= self._cover:
            return
        canvas.saveState()
        canvas.setFillColor(BLACK)
        canvas.setFont("Times-Roman", 10)
        canvas.drawCentredString(A4[0]/2.0, 12*mm, "%d" % doc.page)
        canvas.restoreState()


def tbl(data, widths, header=True, font=9):
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    cmds = [
        ("FONTNAME", (0,0), (-1,-1), "Times-Roman"),
        ("FONTSIZE", (0,0), (-1,-1), font),
        ("TEXTCOLOR", (0,0), (-1,-1), BLACK),
        ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
        ("GRID", (0,0), (-1,-1), 0.5, BLACK),
        ("TOPPADDING", (0,0), (-1,-1), 4),
        ("BOTTOMPADDING", (0,0), (-1,-1), 4),
        ("LEFTPADDING", (0,0), (-1,-1), 5),
    ]
    if header:
        cmds += [
            ("FONTNAME", (0,0), (-1,0), "Times-Bold"),
            ("LINEBELOW", (0,0), (-1,0), 1.0, BLACK),
        ]
    t.setStyle(TableStyle(cmds))
    return t


def bullets(items, style=st_bullet):
    return ListFlowable(
        [ListItem(Paragraph(i, style), leftIndent=12, value="•") for i in items],
        bulletType="bullet", start="•", leftIndent=14, spaceAfter=8)


def P(t, s=st_body): return Paragraph(t, s)
def H1(t): return Paragraph(t, st_h1)
def H2(t): return Paragraph(t, st_h2)
def hr(): return HRFlowable(width="100%", thickness=0.7, color=BLACK, spaceBefore=4, spaceAfter=8)

E = []  # story

# =================================================================== COVER
E += [Spacer(1, 28*mm)]
E += [P("VyroEngine", st_title)]
E += [Spacer(1, 3*mm), hr(), Spacer(1, 10*mm)]
E += [P("Architecture, Design, and Engineering Methodology", st_subtitle)]
E += [P("of a Modern Cross-Platform Game Engine Built From Scratch", st_subtitle)]
E += [Spacer(1, 16*mm)]
E += [P("A Technical Research Paper", st_subtitle)]
E += [Spacer(1, 16*mm)]
E += [P("By", st_author)]
E += [Spacer(1, 4*mm)]
E += [P("<b>Teegulla Gaurav Ganesh</b>", st_author)]
E += [Spacer(1, 22*mm)]
E += [P("June 2026", st_author)]
E += [PageBreak()]

# =================================================================== ABSTRACT
E += [H1("Abstract")]
E += [hr()]
E += [P("Modern interactive entertainment and real-time simulation demand game engines that are simultaneously high-performance, portable, extensible, and approachable. Commercial engines such as Unity and Unreal, and the open-source Godot, each occupy distinct points in this design space. This paper presents <b>VyroEngine</b>, a game engine built from first principles in modern C++23, with a Vulkan-first rendering backend, a data-oriented Entity-Component-System (ECS) core, and an integrated tooling pipeline. We articulate the engine's vision of <i>transparency over magic</i>, in which internal subsystems are explicit, inspectable, and replaceable rather than hidden behind opaque abstractions.", st_abstract)]
E += [P("We describe a twelve-phase development methodology spanning core foundation, ECS architecture, rendering, physics, audio, animation, scripting, editor tooling, networking, advanced graphics, optimization, and production release. For each subsystem we analyze the design constraints, the relevant trade-offs, and how comparable mechanisms are realized in industry-standard engines. We further contribute a complete dependency graph, a solo-developer team structure, a phased 6/12/24-month roadmap, and a quantitative risk analysis. The paper concludes with a reference architecture and a discussion of how a single architect can responsibly scope and sequence the construction of a production-grade engine.", st_abstract)]
E += [Spacer(1, 4*mm)]
E += [P("<b>Keywords:</b> game engine, ECS, Vulkan, real-time rendering, physics simulation, scripting, engine architecture, C++23.", st_abstract)]

# =================================================================== TOC
E += [Spacer(1, 8*mm), H1("Table of Contents"), hr()]
toc_rows = [
    ["1.", "Introduction", "3"],
    ["2.", "Related Work and Industry Landscape", "4"],
    ["3.", "Engine Vision and Requirements", "5"],
    ["4.", "System Architecture Overview", "7"],
    ["5.", "Core Foundation Subsystems", "8"],
    ["6.", "Entity-Component-System Design", "10"],
    ["7.", "Rendering Engine", "11"],
    ["8.", "Physics, Audio, and Animation", "13"],
    ["9.", "Scripting and Editor Tooling", "15"],
    ["10.", "Networking and Advanced Graphics", "16"],
    ["11.", "Development Methodology and Roadmap", "17"],
    ["12.", "Risk Analysis", "18"],
    ["13.", "Conclusion and Future Work", "19"],
    ["", "References", "20"],
]
E += [tbl([["§", "Section", "Page"]] + toc_rows,
          [15*mm, 120*mm, 20*mm], font=10)]
E += [PageBreak()]

# =================================================================== 1 INTRO
E += [H1("1. Introduction")]
E += [hr()]
E += [P("A game engine is a layered software system that abstracts the hardware and operating-system services required to produce real-time interactive applications. At minimum it must orchestrate a deterministic main loop, a rendering pipeline, an input subsystem, an audio subsystem, and a content pipeline that converts authored assets into runtime-optimized data. Contemporary engines extend this baseline with physics simulation, skeletal animation, scripting virtual machines, networking, and full visual editors.", st_body)]
E += [P("Building such a system from scratch is an ambitious undertaking traditionally reserved for well-resourced studios. However, three trends have lowered the barrier: (1) the maturation of cross-platform libraries for windowing, audio, and asset import; (2) the standardization of explicit graphics APIs such as Vulkan that expose GPU behavior directly; and (3) the consolidation of data-oriented design patterns — chiefly the Entity-Component-System — that decouple data layout from behavior and enable cache-friendly, parallelizable execution.", st_body)]
E += [P("VyroEngine is conceived as a <b>pedagogically transparent yet production-capable</b> engine. Its guiding philosophy, <i>transparency over magic</i>, holds that every subsystem should be explicit, documented, and replaceable. Where commercial engines optimize for designer productivity by hiding complexity, VyroEngine deliberately surfaces it, targeting engine programmers and technically inclined teams who require deterministic, inspectable behavior.", st_body)]
E += [H2("1.1 Contributions")]
E += [bullets([
    "A complete software requirements specification and layered architecture for a modern engine built in C++23 with a Vulkan-first backend.",
    "A twelve-phase development methodology decomposed into auditable sub-phases, each with deliverables, technical challenges, and success criteria.",
    "A comparative analysis situating each subsystem against Unity, Unreal Engine, and Godot.",
    "A complete dependency graph, solo-developer team structure, and phased 6/12/24-month roadmap.",
    "A quantitative risk register and a reference architecture diagram.",
])]
E += [H2("1.2 Paper Organization")]
E += [P("Section 2 surveys the industry landscape. Section 3 formalizes the vision and requirements. Section 4 presents the architecture. Sections 5–10 detail subsystems. Section 11 describes the development methodology and roadmap, Section 12 the risk analysis, and Section 13 concludes.", st_body)]
E += [PageBreak()]

# =================================================================== 2 RELATED
E += [H1("2. Related Work and Industry Landscape")]
E += [hr()]
E += [P("Three engines anchor the modern landscape and serve as recurring reference points throughout this paper.", st_body)]
E += [H2("2.1 Unity")]
E += [P("Unity popularized component-based authoring with a managed C# scripting layer atop a native C++ core. Its strengths are an approachable editor, a vast asset store, and broad platform reach. Historically Unity coupled behavior to GameObjects via the MonoBehaviour pattern; its newer Data-Oriented Technology Stack (DOTS) introduces a true archetype-based ECS and the Burst compiler for SIMD-optimized jobs. Unity demonstrates the value of an accessible editor and a managed scripting experience, but also the costs of a managed runtime and historically opaque internals.", st_body)]
E += [H2("2.2 Unreal Engine")]
E += [P("Unreal targets the high-fidelity end of the spectrum. It pairs a C++ gameplay framework with the Blueprint visual scripting system and ships a state-of-the-art deferred renderer featuring Lumen global illumination and Nanite virtualized geometry. Unreal exemplifies a vertically integrated, feature-complete engine, at the cost of a large codebase and a steep learning curve. Its actor/component model and reflection system (UObject) inform VyroEngine's approach to editor-exposed properties.", st_body)]
E += [H2("2.3 Godot")]
E += [P("Godot is a fully open-source engine built around a scene-tree of nodes rather than a pure ECS. It is notable for a lightweight footprint, a permissive MIT license, an integrated editor written in the engine itself, and the GDScript language. Godot proves that a small, transparent, community-driven engine can be production-viable. VyroEngine adopts Godot's openness ethos and its convention of writing engine tooling against the engine's own public API.", st_body)]
E += [H2("2.4 Positioning")]
E += [P("VyroEngine aims to combine Godot's transparency, Unreal's rendering ambition, and Unity's approachability, while making the data-oriented ECS — rather than a scene-tree or GameObject hierarchy — the structural core. The following table summarizes the comparison.", st_body)]
E += [tbl([
    ["Dimension", "Unity", "Unreal", "Godot", "VyroEngine"],
    ["Core model", "GameObject / DOTS", "Actor + Component", "Scene tree", "Pure ECS"],
    ["Language", "C# + C++", "C++ + Blueprint", "C++ + GDScript", "C++23 + Lua"],
    ["Renderer", "URP/HDRP", "Deferred/Lumen", "Vulkan/GL", "Vulkan-first"],
    ["License", "Proprietary", "Proprietary", "MIT", "MIT"],
    ["Philosophy", "Productivity", "Fidelity", "Openness", "Transparency"],
], [30*mm, 28*mm, 30*mm, 28*mm, 30*mm], font=8.5)]
E += [P("Table 1. Comparative positioning of VyroEngine against industry engines.", st_cap)]
E += [PageBreak()]

# =================================================================== 3 VISION
E += [H1("3. Engine Vision and Requirements")]
E += [hr()]
E += [H2("3.1 Vision Statement")]
E += [P("VyroEngine is an open-architecture, data-driven game engine that grants developers full control over every layer of the stack — from memory allocators to render passes — while remaining approachable enough for a small team to operate. It privileges predictable performance and explicit control over hidden automation.", st_body)]
E += [H2("3.2 Target Users")]
E += [bullets([
    "<b>Engine programmers</b> who require low-level, inspectable subsystems.",
    "<b>Indie and small studios</b> seeking a royalty-free, source-available engine.",
    "<b>Researchers and educators</b> teaching real-time graphics and systems programming.",
    "<b>Technical artists</b> building custom rendering and tooling pipelines.",
])]
E += [H2("3.3 Functional Requirements")]
E += [tbl([
    ["ID", "Requirement", "Priority"],
    ["FR-1", "Deterministic fixed/variable-step main loop", "MVP"],
    ["FR-2", "Cross-platform windowing and input abstraction", "MVP"],
    ["FR-3", "Data-oriented ECS with archetype storage", "MVP"],
    ["FR-4", "2D and 3D rendering via Vulkan with GL fallback", "MVP"],
    ["FR-5", "Asset import, hot-reload, and runtime packing", "MVP"],
    ["FR-6", "Rigid-body physics with collision resolution", "MVP"],
    ["FR-7", "Spatial audio playback and mixing", "MVP"],
    ["FR-8", "Lua scripting with hot reload", "MVP"],
    ["FR-9", "Integrated editor (hierarchy, inspector, assets)", "MVP"],
    ["FR-10", "Skeletal animation with blending", "Advanced"],
    ["FR-11", "Client-server networking with replication", "Advanced"],
    ["FR-12", "PBR, dynamic shadows, GI, post-processing", "Advanced"],
], [16*mm, 110*mm, 25*mm], font=8.8)]
E += [P("Table 2. Functional requirements with MVP / advanced classification.", st_cap)]
E += [H2("3.4 Non-Functional Requirements")]
E += [bullets([
    "<b>Performance:</b> maintain 60 FPS at 1080p with 100k visible entities on mid-range hardware.",
    "<b>Portability:</b> Windows, Linux, and macOS from a single CMake build.",
    "<b>Determinism:</b> reproducible simulation given identical inputs and seeds.",
    "<b>Memory safety:</b> custom allocators with leak tracking in debug builds.",
    "<b>Extensibility:</b> subsystems behind interfaces, swappable at compile or runtime.",
])]
E += [H2("3.5 Platform and Toolchain")]
E += [tbl([
    ["Concern", "Choice"],
    ["Language", "C++23 (concepts, ranges, modules where supported)"],
    ["Build system", "CMake 3.28+ with Ninja generator"],
    ["Primary GPU API", "Vulkan 1.3"],
    ["Fallback GPU API", "OpenGL 4.6 (Metal via MoltenVK on macOS)"],
    ["Scripting VM", "Lua 5.4 / LuaJIT"],
    ["Windowing", "SDL3 / GLFW"],
    ["Editor UI", "Dear ImGui"],
    ["Asset import", "Assimp + custom binary pipeline"],
    ["Audio", "OpenAL Soft / miniaudio"],
], [45*mm, 110*mm], font=9)]
E += [P("Table 3. Selected platform and toolchain decisions.", st_cap)]
E += [H2("3.6 Hardware Requirements")]
E += [P("Development targets a workstation with an 8-core CPU, 32 GB RAM, and a Vulkan-1.3-capable GPU. The minimum runtime target is a 4-core CPU, 8 GB RAM, and a GPU exposing Vulkan 1.1 or OpenGL 4.5.", st_body)]
E += [PageBreak()]

# =================================================================== 4 ARCH
E += [H1("4. System Architecture Overview")]
E += [hr()]
E += [P("VyroEngine adopts a strictly layered architecture. Lower layers have no knowledge of higher layers, and cross-cutting services (logging, memory, events) are injected rather than referenced globally. The layering is summarized below.", st_body)]
arch = """
+-------------------------------------------------------------+
|            APPLICATION  (Game / Editor / Tools)             |
+-------------------------------------------------------------+
|   SCRIPTING (Lua)   |   EDITOR (ImGui)   |   NETWORKING      |
+-------------------------------------------------------------+
|   RENDER  |  PHYSICS  |  AUDIO  |  ANIMATION  |  SCENE       |
+-------------------------------------------------------------+
|        ECS CORE  (Entities / Components / Systems)          |
+-------------------------------------------------------------+
|  PLATFORM:  Window | Input | Filesystem | Threads | Time    |
+-------------------------------------------------------------+
|  FOUNDATION: Memory | Logging | Events | Assets | Math      |
+-------------------------------------------------------------+
|                  OS / HARDWARE  (Vulkan, etc.)              |
+-------------------------------------------------------------+
"""
E += [Paragraph(arch.replace(" ", "&nbsp;").replace("\n", "<br/>"), st_code)]
E += [P("Figure 1. VyroEngine layered architecture. Foundation services are available to all higher layers; the ECS core mediates all gameplay subsystems.", st_cap)]
E += [P("The <b>Foundation</b> layer provides allocators, logging, an event bus, asset management, and a math library. The <b>Platform</b> layer abstracts OS services. The <b>ECS core</b> is the structural heart, storing all simulation state. Gameplay subsystems (render, physics, audio, animation) operate as systems over component data. Higher layers — scripting, editor, networking — compose these primitives.", st_body)]
E += [P("Data flows through a single frame tick: input is polled, scripts and gameplay systems mutate components during the variable-step update, the physics system advances on a fixed timestep accumulator, and the render system extracts a view of renderable components to build command buffers. This separation of <i>simulation</i> from <i>presentation</i> is a recurring theme.", st_body)]
E += [PageBreak()]

# =================================================================== 5 CORE
E += [H1("5. Core Foundation Subsystems")]
E += [hr()]
E += [P("Phase 1 establishes the foundation upon which all later work depends. Its sub-systems are intentionally small, heavily tested, and free of external dependencies beyond the standard library and chosen platform shims.", st_body)]
E += [H2("5.1 Memory Management")]
E += [P("Rather than rely solely on the general-purpose allocator, VyroEngine provides specialized allocators: a linear (arena) allocator for per-frame scratch data, a stack allocator for nested scopes, a pool allocator for fixed-size objects such as components, and a free-list allocator for general allocation. Debug builds wrap these with leak tracking and guard bytes. This mirrors the custom allocator strategies used across AAA engines, where control over memory layout is essential to cache performance.", st_body)]
E += [P("Pool&lt;T&gt; alloc -&gt; O(1)  | Arena reset -&gt; O(1) per frame", st_code)]
E += [H2("5.2 Logging System")]
E += [P("A leveled, category-tagged, thread-safe logger writes to console and rotating files. Compile-time log-level stripping ensures verbose logging incurs zero cost in shipping builds. Structured fields enable later ingestion by tooling.", st_body)]
E += [H2("5.3 Event System")]
E += [P("A decoupled publish-subscribe event bus allows subsystems to communicate without direct references. Events are dispatched either immediately or queued for end-of-frame processing. This underpins input propagation, window events, and gameplay messaging, echoing Unreal's delegate system and Godot's signals.", st_body)]
E += [H2("5.4 Input System")]
E += [P("Input is abstracted into devices (keyboard, mouse, gamepad) and an action-mapping layer that binds physical inputs to logical actions. This indirection enables rebinding and platform portability, comparable to Unity's Input System package and Unreal's Enhanced Input.", st_body)]
E += [H2("5.5 Asset Management")]
E += [P("The asset manager provides reference-counted handles, asynchronous loading, and a virtual filesystem that mounts both loose files (development) and packed archives (shipping). Importers convert source formats into engine-native binary representations. A content hash enables hot-reload: when a source file changes, dependents are notified via the event bus.", st_body)]
E += [tbl([
    ["Subsystem", "Key Data Structure", "Primary Concern"],
    ["Memory", "Arena / Pool / Free-list", "Cache locality, no leaks"],
    ["Logging", "Lock-free ring buffer", "Zero-cost in release"],
    ["Events", "Typed dispatch table", "Decoupling"],
    ["Input", "Action map", "Rebinding, portability"],
    ["Assets", "Handle + ref-count", "Async load, hot-reload"],
], [38*mm, 60*mm, 57*mm], font=8.8)]
E += [P("Table 4. Foundation subsystem summary.", st_cap)]
E += [PageBreak()]

# =================================================================== 6 ECS
E += [H1("6. Entity-Component-System Design")]
E += [hr()]
E += [P("The ECS is VyroEngine's defining architectural choice. Entities are lightweight identifiers; components are plain-old-data structs; systems are functions that iterate over entities possessing a given component signature. This separation yields three benefits: cache-friendly contiguous storage, trivial parallelization of independent systems, and composition over inheritance.", st_body)]
E += [H2("6.1 Entity Management")]
E += [P("Entities are 64-bit handles encoding an index and a generation counter. The generation invalidates stale handles when an index is recycled, preventing use-after-free at the gameplay level. A free-list recycles destroyed entity indices.", st_body)]
E += [Paragraph("Entity = { uint32 index; uint32 generation; }".replace(" ", "&nbsp;"), st_code)]
E += [H2("6.2 Component Storage")]
E += [P("VyroEngine uses <b>archetype</b> storage: entities sharing the same component signature are grouped into chunks of tightly packed component arrays. Iterating a system therefore walks contiguous memory, maximizing cache-line utilization. Adding or removing a component migrates the entity between archetypes. This is the same strategy adopted by Unity DOTS and the EnTT library frequently used in open-source engines.", st_body)]
E += [H2("6.3 System Manager")]
E += [P("Systems declare read and write component dependencies. The scheduler builds a dependency graph and dispatches non-conflicting systems across worker threads, while serializing systems that write shared component types. Fixed-step systems (physics) run on an accumulator independent of variable-step systems (input, animation sampling).", st_body)]
E += [H2("6.4 Scene Management")]
E += [P("A scene is a serializable collection of entities and their components. Scenes support additive loading, streaming, and prefab instantiation. Serialization uses a reflective schema so that the editor can save and load arbitrary component types without bespoke code per type.", st_body)]
E += [tbl([
    ["Aspect", "VyroEngine ECS", "Unity DOTS", "Godot (Nodes)"],
    ["Storage", "Archetype chunks", "Archetype chunks", "Object tree"],
    ["Identity", "Index+generation", "Entity struct", "Node pointer"],
    ["Iteration", "Linear over chunks", "Linear (Burst)", "Tree traversal"],
    ["Parallelism", "Dependency graph", "Job system", "Limited"],
], [34*mm, 41*mm, 40*mm, 40*mm], font=8.5)]
E += [P("Table 5. ECS storage strategies compared.", st_cap)]
E += [PageBreak()]

# =================================================================== 7 RENDER
E += [H1("7. Rendering Engine")]
E += [hr()]
E += [P("Rendering is the most complex subsystem and is built atop a thin Render Hardware Interface (RHI) that abstracts the underlying GPU API. The RHI exposes buffers, textures, pipelines, and command queues, with concrete backends for Vulkan (primary) and OpenGL (fallback).", st_body)]
E += [H2("7.1 Render Hardware Interface")]
E += [P("By isolating API specifics behind the RHI, the renderer and all higher layers remain API-agnostic. Vulkan's explicit model — manual synchronization, descriptor sets, and command-buffer recording — is encapsulated so that gameplay code never touches a VkDevice. This mirrors Unreal's RHI and Godot's RenderingDevice abstraction.", st_body)]
E += [H2("7.2 Renderer Architecture")]
E += [P("The renderer follows an <b>extract–prepare–submit</b> pattern. During <i>extract</i>, the render system reads renderable components into an immutable render-view, decoupling it from simulation. During <i>prepare</i>, draw calls are sorted and batched by material and depth to minimize state changes. During <i>submit</i>, command buffers are recorded — potentially on multiple threads — and presented.", st_body)]
E += [H2("7.3 Shader System")]
E += [P("Shaders are authored in GLSL and compiled to SPIR-V offline; a runtime reflection step extracts descriptor layouts so that material parameters bind automatically. A shader-permutation system generates variants for feature toggles (e.g., shadows on/off).", st_body)]
E += [H2("7.4 Texture and Material System")]
E += [P("Textures support mipmapping, compression (BCn), and streaming. Materials bind a shader to a set of textures and uniform parameters, exposed to the editor through reflection.", st_body)]
E += [H2("7.5 Camera System")]
E += [P("Cameras provide perspective and orthographic projections, frustum culling, and multiple-viewport support for the editor. View and projection matrices are computed once per frame and uploaded as a uniform buffer.", st_body)]
E += [H2("7.6 2D and 3D Renderers")]
E += [P("The 2D renderer batches sprites into dynamic vertex buffers with texture atlasing for high throughput. The 3D renderer performs mesh rendering with frustum culling and, in advanced phases, a deferred path supporting physically based shading. Both share the RHI and material systems.", st_body)]
render_flow = "Extract  ->  Cull  ->  Sort/Batch  ->  Record CmdBuf  ->  Submit  ->  Present"
E += [Paragraph(render_flow.replace(" ", "&nbsp;"), st_code)]
E += [P("Figure 2. The render frame pipeline.", st_cap)]
E += [tbl([
    ["Stage", "Responsibility", "Threading"],
    ["Extract", "Snapshot renderable state", "Main thread"],
    ["Cull", "Frustum / occlusion rejection", "Job system"],
    ["Sort/Batch", "Minimize state changes", "Job system"],
    ["Record", "Build command buffers", "Multi-thread"],
    ["Submit", "Queue submit + present", "Render thread"],
], [30*mm, 75*mm, 50*mm], font=8.8)]
E += [P("Table 6. Render pipeline stages and their threading model.", st_cap)]
E += [PageBreak()]

# =================================================================== 8 PHYS/AUDIO/ANIM
E += [H1("8. Physics, Audio, and Animation")]
E += [hr()]
E += [H2("8.1 Physics Engine")]
E += [P("The physics subsystem advances on a fixed timestep to ensure determinism. Collision detection proceeds in two phases: a <b>broad phase</b> using a dynamic bounding-volume hierarchy or spatial hash to cull non-colliding pairs, and a <b>narrow phase</b> using the GJK/EPA algorithms for convex shapes to produce contact manifolds. Collision resolution applies a sequential-impulse solver that iteratively corrects velocities and positions, with constraints (joints) layered atop the same solver.", st_body)]
E += [P("Rigid bodies carry mass, inertia tensors, restitution, and friction. A debug-draw layer visualizes colliders, contact points, and constraint anchors. While VyroEngine implements a custom solver for transparency, the architecture also permits integrating Bullet3 as a drop-in backend, comparable to how Unity wraps PhysX and Godot offers both a built-in and a Jolt backend.", st_body)]
E += [tbl([
    ["Phase", "Technique", "Output"],
    ["Broad phase", "Dynamic BVH / spatial hash", "Candidate pairs"],
    ["Narrow phase", "GJK + EPA", "Contact manifold"],
    ["Resolution", "Sequential impulse solver", "Corrected velocities"],
    ["Constraints", "Joints on impulse solver", "Stable articulation"],
], [33*mm, 67*mm, 55*mm], font=8.8)]
E += [P("Table 7. Physics pipeline stages.", st_cap)]
E += [H2("8.2 Audio Engine")]
E += [P("The audio engine streams and decodes sources, mixes them through a bus graph with per-bus effects, and applies spatialization. Spatial audio attenuates and pans sources according to listener position, with optional HRTF for headphone immersion. A resource layer manages decoded buffers and streaming sources, paralleling the foundation asset manager.", st_body)]
E += [bullets([
    "<b>Playback:</b> streamed and one-shot sources with sample-rate conversion.",
    "<b>Mixing:</b> hierarchical bus graph with volume, EQ, and reverb sends.",
    "<b>Spatialization:</b> distance attenuation, panning, Doppler, optional HRTF.",
    "<b>Resources:</b> ref-counted decoded buffers and streaming handles.",
])]
E += [H2("8.3 Animation System")]
E += [P("Skeletal animation samples keyframed bone transforms, builds a pose, and uploads a skinning-matrix palette consumed by vertex shaders. An animation <b>blend tree</b> mixes multiple clips (e.g., walk/run by speed), and a <b>state machine</b> governs transitions between animation states driven by gameplay parameters. This design follows the Mecanim model in Unity and the AnimGraph in Unreal.", st_body)]
E += [P("Pose = sample(clip, t)  ->  blend(poses, weights)  ->  skinningMatrices[]".replace(" ", "&nbsp;"), st_code)]
E += [P("Figure 3. Animation evaluation pipeline from clip sampling to skinning palette.", st_cap)]
E += [PageBreak()]

# =================================================================== 9 SCRIPT/EDITOR
E += [H1("9. Scripting and Editor Tooling")]
E += [hr()]
E += [H2("9.1 Scripting System")]
E += [P("Gameplay logic is exposed to Lua, chosen for its small footprint, embeddability, and performance (especially under LuaJIT). The engine binds a curated Script API surface — entity creation, component access, input queries, and event subscription — rather than exposing raw engine internals. Scripts hot-reload on file change, preserving iteration speed. The boundary between native and scripted code is explicit, avoiding the performance cliffs of fine-grained interop.", st_body)]
E += [bullets([
    "<b>Lua integration:</b> a sandboxed VM per world with controlled global state.",
    "<b>Script API:</b> stable, versioned bindings generated from reflection metadata.",
    "<b>Hot reload:</b> file-watch triggers re-execution while preserving entity state.",
])]
E += [P("This positions Lua analogously to GDScript in Godot and C# in Unity: a productive iteration language layered over a native core. Performance-critical paths remain in C++.", st_body)]
E += [H2("9.2 Editor Development")]
E += [P("The VyroEditor is built with Dear ImGui and runs against the engine's public API — embodying the same dogfooding principle that makes Godot's editor an engine application. Its panels are composable docking windows.", st_body)]
E += [bullets([
    "<b>Scene hierarchy:</b> a tree view of entities with drag-and-drop reparenting.",
    "<b>Inspector:</b> reflection-driven editing of component fields, with undo/redo.",
    "<b>Asset browser:</b> a filesystem view with import settings and previews.",
    "<b>Gizmos:</b> translate/rotate/scale manipulators rendered in the viewport.",
    "<b>Viewport:</b> an interactive scene view with editor camera controls.",
])]
E += [P("The reflection system is the linchpin connecting ECS components, serialization, scripting bindings, and the inspector: a single schema description drives all four, avoiding duplicated boilerplate. This is analogous to Unreal's UObject reflection (UPROPERTY) and Unity's serialized fields.", st_body)]
E += [tbl([
    ["Editor Panel", "Backed By", "Industry Analogue"],
    ["Hierarchy", "ECS entity graph", "Unity Hierarchy"],
    ["Inspector", "Reflection schema", "Unreal Details panel"],
    ["Asset browser", "Virtual filesystem", "Unity Project window"],
    ["Gizmos", "Editor render pass", "Unreal viewport tools"],
], [38*mm, 55*mm, 62*mm], font=8.8)]
E += [P("Table 8. Editor panels and their backing systems.", st_cap)]
E += [PageBreak()]

# =================================================================== 10 NET/ADV
E += [H1("10. Networking and Advanced Graphics")]
E += [hr()]
E += [H2("10.1 Networking")]
E += [P("VyroEngine targets an authoritative client-server model. The server simulates the canonical world state; clients send inputs and render replicated state. <b>Replication</b> serializes a delta of relevant component state to each client based on relevance filtering (e.g., spatial interest management). <b>Synchronization</b> techniques — client-side prediction, server reconciliation, and entity interpolation — mask latency. This mirrors the replication graph in Unreal and Unity's Netcode for Entities.", st_body)]
E += [tbl([
    ["Concern", "Technique"],
    ["Topology", "Authoritative client-server"],
    ["State transfer", "Delta-compressed component replication"],
    ["Relevance", "Spatial interest management"],
    ["Latency hiding", "Prediction + reconciliation + interpolation"],
], [40*mm, 115*mm], font=8.8)]
E += [P("Table 9. Networking design choices.", st_cap)]
E += [H2("10.2 Advanced Graphics")]
E += [P("The advanced graphics phase elevates fidelity once the Vulkan backend is hardened.", st_body)]
E += [bullets([
    "<b>PBR:</b> a metallic-roughness physically based shading model with image-based lighting.",
    "<b>Shadows:</b> cascaded shadow maps for directional lights and cube maps for point lights.",
    "<b>Post-processing:</b> a configurable stack — tone mapping, bloom, FXAA/TAA, color grading.",
    "<b>Global illumination:</b> an approximate dynamic GI solution (e.g., voxel- or probe-based) inspired by Unreal's Lumen.",
])]
E += [P("These features are deliberately deferred until the core renderer, ECS, and tooling are stable, reflecting the principle that fidelity should not precede foundational correctness.", st_body)]
E += [H2("10.3 Optimization")]
E += [P("A dedicated optimization phase introduces an instrumented profiler (CPU/GPU timings, frame graph), memory-usage analysis, and rendering optimizations such as GPU-driven culling and draw-call batching. Optimization is data-driven: changes are justified by profiler measurements, never speculation.", st_body)]
E += [PageBreak()]

# =================================================================== 11 METHODOLOGY
E += [H1("11. Development Methodology and Roadmap")]
E += [hr()]
E += [P("Construction proceeds through thirteen phases (0–12), each gated by explicit success criteria. The phases follow a strict dependency order: foundation precedes ECS, ECS precedes rendering and physics, and tooling follows a functional runtime.", st_body)]
E += [tbl([
    ["Phase", "Focus", "Est. Duration"],
    ["0", "Research & planning", "2–3 weeks"],
    ["1", "Core foundation", "6–8 weeks"],
    ["2", "ECS architecture", "4–6 weeks"],
    ["3", "Rendering engine", "8–12 weeks"],
    ["4", "Physics engine", "6–8 weeks"],
    ["5", "Audio engine", "3–4 weeks"],
    ["6", "Animation system", "4–6 weeks"],
    ["7", "Scripting system", "3–4 weeks"],
    ["8", "Editor development", "8–10 weeks"],
    ["9", "Networking", "6–8 weeks"],
    ["10", "Advanced graphics", "10–14 weeks"],
    ["11", "Optimization", "4–6 weeks"],
    ["12", "Production release", "4–6 weeks"],
], [18*mm, 90*mm, 47*mm], font=8.8)]
E += [P("Table 10. Phase breakdown with indicative solo-developer durations.", st_cap)]
E += [H2("11.1 Phased Timelines")]
E += [P("<b>6-month horizon:</b> Phases 0–3 — a foundation, ECS, and a working 2D/3D renderer capable of displaying a scene. <b>12-month horizon:</b> Phases 4–8 — physics, audio, animation, scripting, and a usable editor, yielding a minimum-viable engine. <b>24-month horizon:</b> Phases 9–12 — networking, advanced graphics, optimization, and a documented, packaged production release.", st_body)]
E += [H2("11.2 Solo-Developer Team Structure")]
E += [P("Operated by a single architect wearing rotating hats: <i>systems engineer</i> (foundation, ECS), <i>graphics engineer</i> (RHI, renderer), <i>tools engineer</i> (editor), and <i>release engineer</i> (CI, packaging). Work is sequenced so only one role is active at a time, with each phase committed and tagged independently per the project's Git rules.", st_body)]
E += [H2("11.3 Engineering Standards")]
E += [P("The repository codifies engineering rules (the <i>rulz</i>): coding standards, ECS conventions, rendering guidelines, memory discipline, performance budgets, testing requirements, Git workflow, security, and commit-authorship policy. These rules, modeled on the contribution standards of mature open-source engines, ensure consistency as the codebase grows.", st_body)]
E += [PageBreak()]

# =================================================================== 12 RISK
E += [H1("12. Risk Analysis")]
E += [hr()]
E += [P("The dominant risks are scope, Vulkan complexity, and solo-developer bandwidth. Each is rated by likelihood and impact, with a mitigation.", st_body)]
E += [tbl([
    ["Risk", "Likelihood", "Impact", "Mitigation"],
    ["Scope creep", "High", "High", "Strict phase gates; MVP-first cut line"],
    ["Vulkan complexity", "High", "High", "OpenGL fallback first; thin RHI"],
    ["Solo bandwidth", "Medium", "High", "Time-boxed phases; reuse libraries"],
    ["Performance misses", "Medium", "Medium", "Profiler-driven optimization phase"],
    ["Editor over-engineering", "Medium", "Medium", "ImGui; defer polish"],
    ["Asset pipeline churn", "Medium", "Medium", "Stable binary format early"],
    ["Burnout", "Medium", "High", "Sustainable cadence; visible milestones"],
    ["Dependency rot", "Low", "Medium", "Pin versions; vendored deps"],
], [40*mm, 25*mm, 22*mm, 68*mm], font=8.5)]
E += [P("Table 11. Risk register with mitigations.", st_cap)]
E += [P("The single most important mitigation is the <b>MVP cut line</b>: Phases 0–8 constitute a self-contained, shippable engine. Phases 9–12 are explicitly deferred so that an incomplete advanced feature never blocks a usable release. This staging converts an open-ended research project into a sequence of bounded, demonstrable deliverables.", st_body)]
E += [H2("12.1 Dependency Graph (Critical Path)")]
crit = "P0 -> P1 -> P2 -> P3 -> {P4, P5, P6, P7} -> P8 -> P9 -> P10 -> P11 -> P12"
E += [Paragraph(crit.replace(" ", "&nbsp;"), st_code)]
E += [P("Figure 4. Critical-path dependency ordering. Phases 4–7 may interleave once the ECS (P2) and renderer (P3) are stable.", st_cap)]
E += [PageBreak()]

# =================================================================== 13 CONCLUSION
E += [H1("13. Conclusion and Future Work")]
E += [hr()]
E += [P("This paper presented VyroEngine, a modern game engine designed and sequenced from first principles. We argued that a data-oriented ECS core, a Vulkan-first RHI, and a reflection system unifying serialization, scripting, and the editor together form a coherent foundation that balances performance with approachability. We grounded each design decision against the practices of Unity, Unreal, and Godot, and we contributed a complete methodology, dependency graph, roadmap, and risk analysis suitable for a solo architect.", st_body)]
E += [P("VyroEngine's central thesis is that <i>transparency is a feature</i>: by making subsystems explicit and replaceable, the engine becomes both a production tool and a learning artifact. The phased methodology ensures that, at every milestone, a runnable engine exists — never a perpetually half-built monolith.", st_body)]
E += [H2("13.1 Future Work")]
E += [bullets([
    "GPU-driven rendering with mesh shaders and bindless resources.",
    "A job-graph scheduler with work-stealing for finer-grained parallelism.",
    "Virtualized geometry and a global-illumination solution at Lumen/Nanite fidelity.",
    "A visual scripting layer atop the Lua bindings for non-programmers.",
    "Console and WebAssembly platform backends.",
])]
E += [P("By following the roadmap herein, a single dedicated architect can progress from an empty repository to a documented, packaged engine release within a 24-month horizon, with a usable MVP available at the 12-month mark.", st_body)]

# =================================================================== APPENDIX A
E += [PageBreak()]
E += [H1("Appendix A. Repository and Folder Architecture")]
E += [hr()]
E += [P("The repository is organized to separate engine code, tooling, samples, and documentation, enabling independent compilation units and clear ownership boundaries.", st_body)]
folder = """
VyroEngine/
+-- engine/
|   +-- foundation/     (memory, log, events, assets, math)
|   +-- platform/       (window, input, fs, threads, time)
|   +-- ecs/            (entity, component, system, scene)
|   +-- render/         (rhi, renderer, shader, material)
|   +-- physics/        (broadphase, narrowphase, solver)
|   +-- audio/          (playback, mixer, spatial)
|   +-- animation/      (skeleton, blend, statemachine)
|   +-- scripting/      (lua vm, bindings, hotreload)
|   +-- net/            (transport, replication, sync)
+-- editor/             (imgui panels, gizmos, viewport)
+-- runtime/            (game executable entry point)
+-- tools/              (asset pipeline, shader compiler)
+-- samples/            (demo scenes and mini-games)
+-- tests/              (unit + integration tests)
+-- docs/               (papers, design docs, diagrams)
+-- rulz/               (engineering standards)
+-- CMakeLists.txt
+-- README.md
"""
E += [Paragraph(folder.replace(" ", "&nbsp;").replace("\n", "<br/>"), st_code)]
E += [P("Figure 5. VyroEngine repository and folder architecture.", st_cap)]
E += [H2("A.1 Git Workflow")]
E += [P("The project follows trunk-based development on <i>main</i> with short-lived feature branches. Each completed sub-phase is committed atomically and tagged (e.g., <i>v0.3.0-renderer</i>). Commits carry no AI or tool co-authorship per the repository's commit-authorship rule, and follow the Conventional Commits convention (<i>feat:</i>, <i>fix:</i>, <i>docs:</i>, <i>chore:</i>).", st_body)]
E += [tbl([
    ["Convention", "Rule"],
    ["Branching", "Trunk-based; feature branches < 3 days"],
    ["Commits", "Conventional Commits; atomic; signed"],
    ["Tagging", "Semantic version per completed sub-phase"],
    ["Authorship", "Human-only; no AI/bot co-authors"],
    ["CI", "Build + test on every push (Win/Linux/macOS)"],
], [38*mm, 117*mm], font=8.8)]
E += [P("Table 12. Git workflow conventions.", st_cap)]

# =================================================================== APPENDIX B
E += [PageBreak()]
E += [H1("Appendix B. Detailed Phase Deliverables")]
E += [hr()]
E += [P("Each phase below lists its primary goal, key deliverables, and the success criterion that gates progression to the next phase.", st_body)]
E += [tbl([
    ["Phase", "Key Deliverables", "Success Criterion"],
    ["0 Planning", "SRS, architecture, roadmap, rulz", "Approved design docs"],
    ["1 Foundation", "Allocators, logger, events, input, assets", "Window opens; logs; loads asset"],
    ["2 ECS", "Entity mgr, archetype storage, scheduler", "100k entities iterate at 60 FPS"],
    ["3 Render", "RHI, 2D+3D renderer, shaders, camera", "Textured 3D scene on screen"],
    ["4 Physics", "Broad/narrow phase, solver, debug draw", "Stable stacking of rigid bodies"],
    ["5 Audio", "Playback, mixer, spatialization", "3D positional sound plays"],
    ["6 Animation", "Skeletal anim, blend tree, FSM", "Character blends walk/run"],
    ["7 Scripting", "Lua VM, API, hot reload", "Gameplay scripted in Lua"],
    ["8 Editor", "Hierarchy, inspector, assets, gizmos", "Author a scene without code"],
    ["9 Network", "Client-server, replication, sync", "Two clients share a world"],
    ["10 Adv. GFX", "PBR, shadows, post-fx, GI", "PBR scene with dynamic shadows"],
    ["11 Optimize", "Profiler, mem + render optimization", "Meets frame-time budgets"],
    ["12 Release", "Tests, docs, packaging, deploy", "Tagged, packaged 1.0 build"],
], [22*mm, 78*mm, 55*mm], font=8.2)]
E += [P("Table 13. Per-phase deliverables and gating success criteria.", st_cap)]
E += [H2("B.1 Skills Matrix")]
E += [P("The phases collectively demand a broad skill set, summarized below for capacity planning.", st_body)]
E += [tbl([
    ["Domain", "Skills Exercised", "Heaviest Phases"],
    ["Systems", "C++23, memory, concurrency", "1, 2, 11"],
    ["Graphics", "Vulkan, GLSL, linear algebra", "3, 10"],
    ["Simulation", "Collision math, numerical solvers", "4, 6"],
    ["Tooling", "ImGui, UX, reflection", "8"],
    ["Networking", "Sockets, serialization, latency", "9"],
    ["Release", "CMake, CI/CD, docs, packaging", "12"],
], [30*mm, 70*mm, 55*mm], font=8.8)]
E += [P("Table 14. Skills matrix across the development lifecycle.", st_cap)]
E += [P("Taken together, Appendices A and B translate the architectural vision of the preceding sections into an actionable, auditable program of work — the bridge between design intent and day-to-day engineering execution.", st_body)]

# =================================================================== REFERENCES
E += [PageBreak()]
E += [H1("References"), hr()]
refs = [
    "[1] Gregory, J. <i>Game Engine Architecture</i>, 3rd ed. CRC Press, 2018.",
    "[2] Nystrom, R. <i>Game Programming Patterns</i>. Genever Benning, 2014.",
    "[3] Akenine-Möller, T., Haines, E., Hoffman, N. <i>Real-Time Rendering</i>, 4th ed. CRC Press, 2018.",
    "[4] The Khronos Group. <i>Vulkan 1.3 Specification</i>. 2022.",
    "[5] Unity Technologies. <i>Entities (DOTS) Documentation</i>. 2023.",
    "[6] Epic Games. <i>Unreal Engine 5 Documentation: Rendering, Lumen, Nanite</i>. 2023.",
    "[7] Linietsky, J., Manzur, A., et al. <i>Godot Engine Documentation</i>. 2023.",
    "[8] Ericson, C. <i>Real-Time Collision Detection</i>. Morgan Kaufmann, 2005.",
    "[9] Catto, E. \"Iterative Dynamics with Temporal Coherence.\" GDC, 2005.",
    "[10] Ierusalimschy, R. <i>Programming in Lua</i>, 4th ed. 2016.",
    "[11] Caudron, M., et al. \"EnTT: A Fast and Reliable ECS for C++.\" 2023.",
    "[12] Glassner, A. (ed.) <i>Graphics Gems</i>. Academic Press, 1990.",
    "[13] Fiedler, G. \"Networked Physics\" and \"Snapshot Interpolation.\" Gaffer On Games, 2015.",
    "[14] Pharr, M., Jakob, W., Humphreys, G. <i>Physically Based Rendering</i>, 4th ed. 2023.",
]
for r in refs:
    E += [Paragraph(r, st_ref)]

# build
doc = DocTemplate(OUT, pagesize=A4,
                  leftMargin=20*mm, rightMargin=20*mm,
                  topMargin=18*mm, bottomMargin=18*mm,
                  title="VyroEngine: Architecture and Design of a Modern Game Engine",
                  author="Teegulla Gaurav Ganesh")
doc.build(E)
print("WROTE", OUT)
