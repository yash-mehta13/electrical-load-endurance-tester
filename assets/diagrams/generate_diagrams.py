"""Generates the SVG diagrams in this directory (stdlib only): python3 generate_diagrams.py

The SVG files are the published artefacts; this script is the maintained
source for their layout. Status colours: blue = code exists but is not
hardware-verified, green = host-unit-tested logic, grey dashed = planned,
amber dashed = unresolved hardware choice. Keep the diagrams honest: do not
change a block's status without evidence (see docs/traceability.md).
"""
import html, os
OUT = os.path.dirname(os.path.abspath(__file__))
INK = "#1b1f24"; MUTE = "#4a5560"
# status styles: (fill, stroke, dash)
ST = {
 "exists":  ("#dbeafe", "#1d4ed8", ""),        # code exists in repo, not hardware-verified
 "tested":  ("#dcfce7", "#15803d", ""),        # host unit-tested logic
 "planned": ("#f1f3f5", "#5c6773", "7 4"),     # planned, not implemented
 "option":  ("#fef3c7", "#b45309", "7 4"),     # unresolved hardware choice
 "power":   ("#fee2e2", "#b91c1c", ""),        # power path
 "ext":     ("#ffffff", "#1b1f24", ""),        # external equipment
 "warn":    ("#fff1f2", "#be123c", ""),
}
class S:
    def __init__(s, w, h, title, desc):
        s.w, s.h, s.p = w, h, []
        s.title, s.desc = title, desc
    def add(s, x): s.p.append(x)
    def text(s, x, y, t, size=14, w="400", anchor="start", fill=INK, style=""):
        s.add(f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{w}" text-anchor="{anchor}" fill="{fill}" {style}>{html.escape(t)}</text>')
    def box(s, x, y, w, h, lines, st="exists", size=14, title=None, rx=8, align="middle"):
        f, k, d = ST[st]
        da = f' stroke-dasharray="{d}"' if d else ""
        s.add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{f}" stroke="{k}" stroke-width="2"{da}/>')
        n = len(lines) + (1 if title else 0)
        lh = size + 4
        y0 = y + h/2 - (n*lh)/2 + size
        cx = x + w/2 if align == "middle" else x + 10
        if title:
            s.text(cx, y0, title, size, "700", align); y0 += lh
        for l in lines:
            s.text(cx, y0, l, size-1 if title else size, "400", align, MUTE if title else INK); y0 += lh
    def arrow(s, pts, label=None, lx=None, ly=None, color=INK, dash="", both=False, anchor="middle", lsize=12):
        d = "M" + " L".join(f"{x},{y}" for x, y in pts)
        da = f' stroke-dasharray="{dash}"' if dash else ""
        ms = ' marker-start="url(#ah)"' if both else ""
        s.add(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="2"{da} marker-end="url(#ah)"{ms}/>')
        if label:
            x, y = (lx, ly) if lx is not None else ((pts[0][0]+pts[-1][0])/2, (pts[0][1]+pts[-1][1])/2 - 6)
            s.text(x, y, label, lsize, "400", anchor, color if color != INK else MUTE)
    def legend(s, x, y, items):
        for i, (st, label) in enumerate(items):
            f, k, d = ST[st]; da = f' stroke-dasharray="{d}"' if d else ""
            yy = y + i*24
            s.add(f'<rect x="{x}" y="{yy}" width="28" height="16" rx="3" fill="{f}" stroke="{k}" stroke-width="2"{da}/>')
            s.text(x+38, yy+13, label, 12.5, "400", "start", MUTE)
    def save(s, name):
        body = "\n".join(s.p)
        svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {s.w} {s.h}" width="{s.w}" height="{s.h}" font-family="Helvetica, Arial, sans-serif" role="img" aria-labelledby="t d">
<title id="t">{html.escape(s.title)}</title>
<desc id="d">{html.escape(s.desc)}</desc>
<defs><marker id="ah" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="{INK}"/></marker></defs>
<rect x="0" y="0" width="{s.w}" height="{s.h}" fill="#ffffff"/>
<rect x="1" y="1" width="{s.w-2}" height="{s.h-2}" fill="none" stroke="#d0d7de" stroke-width="2" rx="10"/>
{body}
</svg>
'''
        open(os.path.join(OUT, name), "w", encoding="utf-8").write(svg)

LEG_STD = [("exists", "Code exists in repo; not verified on hardware"),
           ("tested", "Logic exists and is host-unit-tested; not integrated"),
           ("planned", "Planned / not implemented"),
           ("option", "Unresolved hardware choice")]

# ---------------- A. system architecture ----------------
a = S(1000, 730, "System architecture (intended)", "Block diagram of the intended single-channel 12 V DC endurance tester: bench supply, protection, switching stage options, load, sensing, ESP32, UART and CAN. Nothing is hardware-verified.")
a.text(24, 34, "System architecture - intended design (Phase 0: nothing here is hardware-verified)", 17, "700")
a.box(24, 70, 130, 70, ["current-limited", "12 V DC bench PSU"], "ext", size=13)
a.box(214, 70, 130, 70, ["Fuse / PTC", "rating TBD"], "planned")
a.box(404, 56, 220, 98, ["A: BTS7960 module", "B: N-MOSFET + gate driver", "(not decided)"], "option", title="Switching stage (high-side)")
a.box(684, 70, 140, 70, ["relay / solenoid /", "motor / lamp"], "ext", title="12 V DC load")
a.box(874, 70, 102, 70, ["flyback diode", "(inductive)"], "planned", size=13)
a.arrow([(154,105),(214,105)]); a.arrow([(344,105),(404,105)], "+12 V", lx=374, ly=96); a.arrow([(624,105),(684,105)], "switched", lx=654, ly=96, lsize=11)
a.arrow([(824,105),(874,105)])
# sense taps
a.add(f'<path d="M119,176 L559,176" fill="none" stroke="{INK}" stroke-width="2" stroke-dasharray="5 3"/>')
a.arrow([(559,154),(559,176)], dash="5 3")
a.text(132, 168, "measurement tap points (positions TBD)", 12, "400", "start", MUTE)
a.box(24, 200, 190, 96, ["I2C 0x40 (default)", "bus V + shunt current", "shunt value TBD"], "exists", title="INA226 module")
a.box(244, 200, 190, 96, ["resistor divider ->", "ADC1 (GPIO34 default)", "resistor values TBD"], "exists", title="Voltage divider")
a.box(464, 200, 190, 96, ["1-Wire, 4.7k pull-up", "on switch / heatsink", "(pull-up not in BOM)"], "exists", title="DS18B20 probe")
a.arrow([(119,176),(119,200)], dash="5 3"); a.arrow([(339,176),(339,200)], dash="5 3"); a.arrow([(559,176),(559,200)], dash="5 3")
a.box(160, 350, 520, 120, ["Phase 0: standalone bring-up tests only (firmware/bringup)", "Product firmware (firmware/main): not started", "Gate-control GPIO: unassigned"], "exists", title="ESP32 DevKit (WROOM-32)", size=14)
a.arrow([(119,296),(119,330),(250,330),(250,350)], "I2C SDA/SCL", lx=135, ly=322, both=True, anchor="start")
a.arrow([(339,296),(339,350)], "ADC", lx=352, ly=330, anchor="start")
a.arrow([(559,296),(559,350)], "1-Wire", lx=572, ly=330, anchor="start", both=True)
a.arrow([(677,350),(677,176),(610,176),(610,154)], "gate / enable control", lx=687, ly=230, anchor="start", dash="7 4", color="#b45309")
a.text(687, 248, "(GPIO + hardware pull to OFF", 12, "400", "start", "#b45309"); a.text(687, 264, "level required; both TBD)", 12, "400", "start", "#b45309")
a.box(724, 380, 252, 130, ["No independent hardware trip is", "defined yet beyond fuse/PTC and, for", "option A, the BTS7960's own protection", "(unverified). A firmware load-OFF", "is not physical protection."], "warn", title="Protection limitation", size=12.5)
a.box(24, 560, 190, 76, ["USB-serial console", "115200 baud"], "exists", title="UART0")
a.box(244, 560, 190, 76, ["SN65HVD230", "3.3 V, TX/RX GPIO TBD"], "planned", title="CAN transceiver")
a.box(464, 560, 190, 76, ["CANH/CANL, 120 ohm x2", "termination TBD"], "planned", title="CAN bus")
a.box(684, 560, 190, 76, ["PC + CAN tool", "(adapter in BOM)"], "planned", title="USB-CAN adapter")
a.arrow([(119,470),(119,560)], "UART", lx=132, ly=520, anchor="start", both=True)
a.arrow([(339,470),(339,560)], "TWAI", lx=352, ly=520, anchor="start", both=True)
a.arrow([(434,598),(464,598)], both=True); a.arrow([(654,598),(684,598)], both=True)
a.text(24, 672, "Power-return / common-ground topology and any isolation barrier are not yet defined (see docs/hardware_architecture.md).", 12, "400", fill=MUTE)
a.legend(24, 686, LEG_STD[:2]); a.legend(500, 686, LEG_STD[2:])
a.h = 740
a.save("system_architecture.svg")

# ---------------- B. state machine ----------------
b = S(1000, 700, "Endurance state machine", "State diagram of the endurance scheduler core: BOOT, IDLE, RUNNING_ON, RUNNING_OFF, COMPLETE and FAULT with transitions.")
b.text(24, 34, "Endurance state machine - implemented in firmware/core, host-unit-tested only", 17, "700")
b.box(40, 90, 150, 56, ["load OFF"], "tested", title="BOOT")
b.box(300, 90, 150, 56, ["load OFF"], "tested", title="IDLE")
b.add(f'<rect x="530" y="68" width="250" height="262" rx="12" fill="none" stroke="{MUTE}" stroke-width="1.5" stroke-dasharray="4 4"/>')
b.text(542, 86, "RUNNING (either phase)", 11.5, "400", "start", MUTE)
b.box(560, 100, 150, 56, ["load ON"], "tested", title="RUNNING_ON")
b.box(560, 250, 150, 56, ["load OFF"], "tested", title="RUNNING_OFF")
b.box(560, 410, 150, 56, ["load OFF"], "tested", title="COMPLETE")
b.box(40, 540, 440, 70, ["load OFF, latched; fault code and cycles-at-fault recorded;", "no automatic restart"], "warn", title="FAULT", size=13)
b.arrow([(190,118),(300,118)], "self-test passed", lx=245, ly=108, lsize=11)
b.arrow([(450,128),(560,128)], "start", lx=505, ly=118)
b.text(505, 146, "(valid profile)", 11, "400", "middle", MUTE)
b.arrow([(635,156),(635,250)], "on_ms elapsed", lx=645, ly=208, anchor="start")
b.arrow([(710,278),(750,278),(750,128),(710,128)], "off_ms elapsed,", lx=792, ly=190, anchor="middle", lsize=11)
b.text(792, 206, "cycles < target", 11, "400", "middle", MUTE); b.text(792, 222, "(cycle counted)", 11, "400", "middle", MUTE)
b.arrow([(635,306),(635,410)], "off_ms elapsed,", lx=645, ly=350, anchor="start", lsize=11)
b.text(645, 366, "cycles == target", 11, "400", "start", MUTE); b.text(645, 382, "(last cycle counted)", 11, "400", "start", MUTE)
b.arrow([(530,200),(420,200),(420,146)], "stop", lx=475, ly=190, lsize=11)
b.text(420, 228, "partial cycle not", 11, "400", "middle", MUTE); b.text(420, 243, "counted; load OFF", 11, "400", "middle", MUTE)
b.text(635, 490, "new start -> RUNNING_ON", 11.5, "400", "middle", MUTE); b.text(635, 506, "(counters reset)", 11.5, "400", "middle", MUTE)
b.arrow([(115,540),(115,146)], None, color="#be123c")
b.text(125, 330, "any state: fault raised", 12, "700", "start", "#be123c")
b.text(125, 348, "(first fault code is kept;", 11.5, "400", "start", "#be123c"); b.text(125, 364, "load OFF at once)", 11.5, "400", "start", "#be123c")
b.arrow([(360,540),(360,146)], None, color="#b45309")
b.text(350, 420, "operator-authorized", 11.5, "700", "end", "#b45309"); b.text(350, 437, "clear -> IDLE (does not", 11.5, "400", "end", "#b45309"); b.text(350, 453, "resume the test)", 11.5, "400", "end", "#b45309")
b.box(520, 540, 456, 76, ["Not in the core yet: SELF_TEST, READY, STOPPING (needs load-OFF", "feedback), RECOVERING. Retry policy intentionally undefined -", "see docs/fault_policy.md"], "planned", size=12.5)
b.legend(24, 632, [LEG_STD[1], ("warn", "Fault / latched state")]); b.legend(520, 632, [LEG_STD[2]])
b.save("state_machine.svg")

# ---------------- C. fault flow ----------------
c = S(1000, 730, "Fault detection and response flow", "Flow from measurement acquisition through validity checks, thresholds, debounce, classification, safe state, logging and recovery. Thresholds and classification rules are unresolved.")
c.text(24, 34, "Fault detection and response - proposed flow (thresholds and classification rules unresolved)", 17, "700")
W = 340
c.box(40, 70, W, 64, ["INA226 (I2C), divider (ADC), DS18B20 (1-Wire),", "sampled at independent rates (FR3)"], "planned", title="1  Acquire measurements", size=13)
c.box(40, 170, W, 70, ["stale sample, I2C/CRC error, out of range,", "DS18B20 85.0 C reset value"], "planned", title="2  Validate samples", size=13)
c.box(40, 280, W, 70, ["compare against a per-fault threshold;", "thresholds TBD (no rated limits recorded)"], "option", title="3  Threshold compare", size=13)
c.box(40, 390, W, 70, ["N consecutive samples (firmware/core/debounce.c,", "host-tested); N per fault TBD"], "tested", title="4  Debounce", size=13)
c.box(40, 500, W, 70, ["transient vs hard: rules NOT defined yet;", "no fault is treated as safe to retry"], "option", title="5  Classify", size=13)
for y1, y2 in [(134,170),(240,280),(350,390),(460,500)]:
    c.arrow([(210,y1),(210,y2)])
c.text(222, 266, "valid", 12, "400", "start", MUTE)
c.text(222, 482, "tripped", 12, "400", "start", MUTE)
c.text(392, 428, "not tripped: test continues", 11.5, "400", "start", MUTE)
c.box(470, 170, 260, 70, ["invalid data is itself a fault;", "never read as 'no fault'"], "warn", title="SENSOR_INVALID", size=13)
c.arrow([(380,205),(470,205)], "invalid", lx=425, ly=196, lsize=11)
c.box(470, 500, 260, 70, ["command load OFF; state FAULT (latched);", "core host-tested, GPIO polarity TBD"], "tested", title="6  Safe state", size=13)
c.arrow([(380,535),(470,535)])
c.arrow([(600,240),(600,500)], dash="5 3")
c.box(770, 500, 206, 70, ["UART line + CAN 0x101", "(planned): values, cycle no."], "planned", title="7  Log event", size=13)
c.arrow([(730,535),(770,535)])
c.box(770, 360, 206, 90, ["latched until operator command;", "auto-retry only with a documented", "rationale (none yet)"], "option", title="8  Recovery policy", size=12.5)
c.arrow([(873,500),(873,450)])
c.box(40, 610, 936, 50, ["Detecting a fault in software is not physical protection: fuse/PTC, current-limited PSU and any hardware trip must act independently."], "warn", size=13)
c.legend(40, 676, [LEG_STD[1], LEG_STD[2]]); c.legend(500, 676, [LEG_STD[3]])
c.h = 730
c.save("fault_response.svg")

# ---------------- D. CAN ----------------
d = S(1000, 480, "CAN communication topology (planned)", "Planned topology: ESP32 with SN65HVD230 transceiver on a CAN bus connected through a USB-CAN adapter to a PC. No CAN hardware or code exists yet.")
d.text(24, 34, "CAN communication - planned topology (no CAN code or hardware exists yet)", 17, "700")
d.box(24, 90, 200, 90, ["TWAI controller", "TX / RX GPIO unassigned", "bit rate undecided"], "planned", title="ESP32")
d.box(284, 90, 190, 90, ["3.3 V part", "standby / slope pins TBD"], "planned", title="SN65HVD230")
d.box(534, 90, 190, 90, ["120 ohm at each end", "(module may include one)"], "planned", title="CAN bus")
d.box(784, 90, 192, 90, ["in BOM; model not", "recorded"], "planned", title="USB-CAN adapter")
d.arrow([(224,135),(284,135)], "TX/RX", lx=254, ly=126, both=True)
d.arrow([(474,135),(534,135)], "CANH/L", lx=504, ly=126, both=True)
d.arrow([(724,135),(784,135)], both=True)
d.box(784, 240, 192, 70, ["SavvyCAN / candump", "(host side)"], "planned", title="PC tools")
d.arrow([(880,180),(880,240)], both=True)
d.box(534, 240, 190, 70, ["not in BOM, not in roadmap", "- would need its own plan"], "option", title="Optional 2nd node")
d.arrow([(629,180),(629,240)], both=True, dash="5 3")
d.text(24, 232, "Proposed frames (IDs unconfirmed, no implementation):", 14, "700")
rows = [("0x100", "Status, 10 Hz, ESP32 -> bus"), ("0x101", "Fault event, on event, ESP32 -> bus"), ("0x200", "Command, bus -> ESP32")]
for i, (i_d, t) in enumerate(rows):
    d.text(32, 262 + i*24, i_d, 13, "700"); d.text(100, 262 + i*24, t, 13, "400", fill=MUTE)
d.text(24, 360, "Ground reference between the CAN nodes and the 12 V domain is not yet defined; isolation is not claimed.", 12.5, "400", fill=MUTE)
d.legend(24, 392, [LEG_STD[2], LEG_STD[3]])
d.save("can_topology.svg")

# ---------------- E. timing ----------------
e = S(1000, 430, "Endurance test timing (illustrative)", "Illustrative timing diagram: ON and OFF intervals for three cycles, with the cycle counter incrementing at the end of each OFF interval, then COMPLETE. Not measured data.")
e.text(24, 34, "Endurance cycle timing - ILLUSTRATIVE example (on_ms, off_ms, target_cycles = 3), not measured data", 17, "700")
x0 = 230; on = 90; off = 130
e.text(24, 128, "load command", 14, "700"); e.text(24, 146, "(true only in RUNNING_ON)", 11.5, "400", fill=MUTE)
yh, yl = 100, 160
pts = [(x0, yl)]; x = x0
marks = []
for i in range(3):
    pts += [(x, yh), (x+on, yh), (x+on, yl), (x+on+off, yl)]
    marks.append((x, x+on, x+on+off)); x += on + off
e.add('<path d="M' + " L".join(f"{px},{py}" for px, py in pts) + f'" fill="none" stroke="#1d4ed8" stroke-width="3"/>')
for i, (s0, s1, s2) in enumerate(marks):
    e.add(f'<rect x="{s0}" y="{yh}" width="{on}" height="{yl-yh}" fill="#dbeafe" opacity="0.55"/>')
    e.text((s0+s1)/2, 92, "ON", 13, "700", "middle", "#1d4ed8"); e.text((s1+s2)/2, 182, "OFF", 13, "700", "middle", MUTE)
    e.add(f'<line x1="{s2}" y1="88" x2="{s2}" y2="300" stroke="#8c959f" stroke-dasharray="4 4"/>')
    e.add(f'<line x1="{s0}" y1="88" x2="{s0}" y2="300" stroke="#8c959f" stroke-dasharray="4 4"/>')
e.add(f'<line x1="{x}" y1="88" x2="{x}" y2="300" stroke="#8c959f" stroke-dasharray="4 4"/>')
e.arrow([(x0,208),(x0+on,208)], "on_ms", lx=x0+on/2, ly=224, both=True, lsize=12)
e.arrow([(x0+on,208),(x0+on+off,208)], "off_ms", lx=x0+on+off/2, ly=224, both=True, lsize=12)
e.text(24, 268, "cycles completed", 14, "700"); e.text(24, 286, "(counter)", 11.5, "400", fill=MUTE)
cx = x0
for i, (s0, s1, s2) in enumerate(marks):
    e.add(f'<line x1="{s0 if i==0 else marks[i-1][2]}" y1="272" x2="{s2}" y2="272" stroke="#15803d" stroke-width="3"/>')
    e.add(f'<circle cx="{s2}" cy="272" r="5" fill="#15803d"/>')
    e.text(s2+8, 262, f"{i+1}", 14, "700", "start", "#15803d")
e.add(f'<line x1="{x0}" y1="272" x2="{x0}" y2="272"/>')
e.text(x0+8, 262, "0", 14, "700", "start", "#15803d")
e.text(x-50, 322, "COMPLETE: load stays OFF", 12.5, "700", "middle", "#15803d")
e.text(24, 360, "A cycle counts only when its ON interval and the following OFF interval have both fully elapsed.", 13, "400")
e.text(24, 380, "A stop or a fault during either interval drives the load OFF immediately and the partial cycle is not counted.", 13, "400")
e.text(24, 406, "Interval values are placeholders; real limits depend on the load and switch ratings (not yet recorded).", 12.5, "400", fill=MUTE)
e.save("endurance_timing.svg")

# ---------------- F. pin map ----------------
f = S(1000, 560, "Phase 0 default pin map", "Software default GPIO assignments from the bring-up Kconfig. Not verified on hardware. Load-control and CAN pins are unassigned.")
f.text(24, 34, "Phase 0 bring-up default pin map - software defaults (menuconfig), wiring NOT verified on hardware", 17, "700")
f.box(380, 70, 240, 330, ["WROOM-32 DevKit", "(38-pin)"], "exists", title="ESP32")
pins = [
 (95,  "GPIO2",  "onboard LED (test: blink)", "L", "exists"),
 (150, "GPIO34", "ADC1_CH6 <- divider tap (input-only)", "L", "exists"),
 (205, "GPIO21", "I2C SDA -> INA226", "L", "exists"),
 (260, "GPIO22", "I2C SCL -> INA226", "L", "exists"),
 (315, "GPIO4",  "1-Wire DATA <-> DS18B20", "L", "exists"),
 (95,  "GPIO ?", "load enable / gate control", "R", "option"),
 (150, "GPIO ?", "TWAI TX -> SN65HVD230", "R", "option"),
 (205, "GPIO ?", "TWAI RX <- SN65HVD230", "R", "option"),
 (260, "UART0",  "console / flashing (USB bridge)", "R", "exists"),
]
for y, g, t, side, st in pins:
    fl, k, da = ST[st]; dsh = f' stroke-dasharray="{da}"' if da else ""
    if side == "L":
        f.add(f'<rect x="24" y="{y-18}" width="316" height="38" rx="6" fill="{fl}" stroke="{k}" stroke-width="2"{dsh}/>')
        f.text(36, y+6, g, 14, "700"); f.text(104, y+6, t, 12.5, "400", fill=MUTE)
        f.add(f'<line x1="340" y1="{y}" x2="380" y2="{y}" stroke="{INK}" stroke-width="2"/>')
    else:
        f.add(f'<rect x="660" y="{y-18}" width="316" height="38" rx="6" fill="{fl}" stroke="{k}" stroke-width="2"{dsh}/>')
        f.text(672, y+6, g, 14, "700"); f.text(740, y+6, t, 12.5, "400", fill=MUTE)
        f.add(f'<line x1="620" y1="{y}" x2="660" y2="{y}" stroke="{INK}" stroke-width="2"/>')
f.text(24, 430, "Pin notes (from ESP32 documentation; confirm for your board):", 13.5, "700")
notes = ["GPIO34-39 are input-only; GPIO6-11 are tied to flash - do not use.",
         "GPIO0, 2, 5, 12, 15 are strapping pins: do not let external circuits drive them at reset.",
         "At reset GPIOs are not guaranteed to hold a level: the switching stage needs a hardware pull to its OFF level.",
         "ADC2 pins conflict with Wi-Fi and are avoided; the ADC test accepts ADC1 pins only (GPIO32-39).",
         "Never connect 12 V to any GPIO; the divider output must stay within the ADC range with margin."]
for i, n in enumerate(notes):
    f.text(32, 452 + i*19, "- " + n, 12.5, "400", fill=MUTE)
f.add(f'<rect x="700" y="500" width="28" height="16" rx="3" fill="{ST["exists"][0]}" stroke="{ST["exists"][1]}" stroke-width="2"/>'); f.text(738, 513, "Default exists in bring-up code", 12.5, "400", fill=MUTE)
f.add(f'<rect x="700" y="524" width="28" height="16" rx="3" fill="{ST["option"][0]}" stroke="{ST["option"][1]}" stroke-width="2" stroke-dasharray="7 4"/>'); f.text(738, 537, "Unassigned / unresolved", 12.5, "400", fill=MUTE)
f.save("pin_map.svg")
print("ok")
