import { useState, useRef } from 'react'

// ── Metallic cyberpunk background ──────────────────────────────────────────
function MetallicBackground() {
  return (
    <div style={{ position: 'absolute', inset: 0, zIndex: 0, overflow: 'hidden', borderRadius: 6 }}>
      {/* SVG noise/grain layer — brushed-metal turbulence */}
      <svg
        xmlns="http://www.w3.org/2000/svg"
        style={{ position: 'absolute', inset: 0, width: '100%', height: '100%' }}
      >
        <defs>
          {/* Horizontal-streak fractal noise → brushed steel grain */}
          <filter id="bg-metal" x="0%" y="0%" width="100%" height="100%" colorInterpolationFilters="sRGB">
            <feTurbulence
              type="fractalNoise"
              baseFrequency="0.55 0.04"
              numOctaves="5"
              seed="8"
              result="raw"
            />
            {/* Tint noise into dark navy-blue range */}
            <feColorMatrix
              in="raw"
              type="matrix"
              values="0.06 0.09 0.18 0 0.03
                      0.07 0.10 0.20 0 0.04
                      0.12 0.16 0.36 0 0.07
                      0    0    0   0.85 0"
              result="metalBase"
            />
            {/* Second turbulence pass for fine grain scratch marks */}
            <feTurbulence
              type="turbulence"
              baseFrequency="0.03 0.8"
              numOctaves="2"
              seed="14"
              result="scratches"
            />
            <feColorMatrix
              in="scratches"
              type="matrix"
              values="0 0 0 0 0.08
                      0 0 0 0 0.10
                      0 0 0 0 0.22
                      0 0 0 0.28 0"
              result="scratchTinted"
            />
            <feComposite in="metalBase" in2="scratchTinted" operator="over" result="combined" />
          </filter>

          {/* Cyan highlight glow */}
          <radialGradient id="bg-cyan" cx="22%" cy="48%" r="48%">
            <stop offset="0%" stopColor="#00aaff" stopOpacity="0.28" />
            <stop offset="55%" stopColor="#0066cc" stopOpacity="0.08" />
            <stop offset="100%" stopColor="#0066cc" stopOpacity="0" />
          </radialGradient>

          {/* Dark purple highlight glow */}
          <radialGradient id="bg-purple" cx="75%" cy="60%" r="44%">
            <stop offset="0%" stopColor="#6600dd" stopOpacity="0.26" />
            <stop offset="50%" stopColor="#4400aa" stopOpacity="0.08" />
            <stop offset="100%" stopColor="#4400aa" stopOpacity="0" />
          </radialGradient>

          {/* Second purple patch, upper-right */}
          <radialGradient id="bg-purple2" cx="82%" cy="12%" r="32%">
            <stop offset="0%" stopColor="#4400bb" stopOpacity="0.20" />
            <stop offset="100%" stopColor="#4400bb" stopOpacity="0" />
          </radialGradient>

          {/* Second cyan patch, lower-left */}
          <radialGradient id="bg-cyan2" cx="10%" cy="88%" r="30%">
            <stop offset="0%" stopColor="#0055cc" stopOpacity="0.18" />
            <stop offset="100%" stopColor="#0055cc" stopOpacity="0" />
          </radialGradient>

          {/* Vignette — must be in defs */}
          <radialGradient id="bg-vignette" cx="50%" cy="50%" r="70%">
            <stop offset="0%" stopColor="transparent" />
            <stop offset="100%" stopColor="#000008" stopOpacity="0.75" />
          </radialGradient>
        </defs>

        {/* Deep base */}
        <rect width="100%" height="100%" fill="#080d1c" />

        {/* Metallic grain */}
        <rect width="100%" height="100%" filter="url(#bg-metal)" />

        {/* Vignette */}
        <rect width="100%" height="100%" fill="url(#bg-vignette)" />

        {/* Color bleeds */}
        <rect width="100%" height="100%" fill="url(#bg-cyan)" />
        <rect width="100%" height="100%" fill="url(#bg-purple)" />
        <rect width="100%" height="100%" fill="url(#bg-purple2)" />
        <rect width="100%" height="100%" fill="url(#bg-cyan2)" />
      </svg>

      {/* CSS diagonal metallic streak lines */}
      <div
        style={{
          position: 'absolute',
          inset: 0,
          backgroundImage: [
            'repeating-linear-gradient(72deg, transparent 0px, rgba(0,80,160,0.06) 1px, transparent 2px, transparent 38px)',
            'repeating-linear-gradient(68deg, transparent 0px, rgba(40,0,100,0.05) 1px, transparent 2px, transparent 52px)',
            'repeating-linear-gradient(75deg, transparent 0px, rgba(0,160,255,0.025) 1px, transparent 1.5px, transparent 70px)',
          ].join(', '),
        }}
      />

      {/* Fine horizontal scan-line grid over everything */}
      <div
        style={{
          position: 'absolute',
          inset: 0,
          backgroundImage:
            'repeating-linear-gradient(0deg, rgba(0,0,0,0.18) 0px, rgba(0,0,0,0.18) 1px, transparent 1px, transparent 3px)',
        }}
      />
    </div>
  )
}

// ── Rotary Knob ────────────────────────────────────────────────────────────
interface KnobProps {
  value: number
  onChange: (v: number) => void
  accent: string
  glowColor: string
  size?: number
  id: string
}

function Knob({ value, onChange, accent, glowColor, size = 140, id }: KnobProps) {
  const drag = useRef({ active: false, startY: 0, startVal: 0 })

  const onPointerDown = (e: React.PointerEvent<SVGSVGElement>) => {
    drag.current = { active: true, startY: e.clientY, startVal: value }
    e.currentTarget.setPointerCapture(e.pointerId)
  }
  const onPointerMove = (e: React.PointerEvent<SVGSVGElement>) => {
    if (!drag.current.active) return
    const delta = (drag.current.startY - e.clientY) / 300
    onChange(Math.max(0, Math.min(1, drag.current.startVal + delta)))
  }
  const onPointerUp = () => { drag.current.active = false }

  const cx = size / 2, cy = size / 2
  const r = size * 0.34
  const sw = size * 0.075

  // Arc geometry: track from 135° (7-o'clock) clockwise 270° to 45° (5-o'clock)
  const TRACK_START = 135
  const pol = (deg: number, radius: number): [number, number] => {
    const rad = (deg * Math.PI) / 180
    return [cx + radius * Math.cos(rad), cy + radius * Math.sin(rad)]
  }

  const arcPath = (fromDeg: number, toDegRaw: number, radius: number): string | null => {
    const to = ((toDegRaw % 360) + 360) % 360
    const sweep = ((to - fromDeg) + 360) % 360
    if (sweep < 0.5) return null
    const [x1, y1] = pol(fromDeg, radius)
    const [x2, y2] = pol(to, radius)
    const lg = sweep > 180 ? 1 : 0
    return `M${x1.toFixed(2)} ${y1.toFixed(2)} A${radius} ${radius} 0 ${lg} 1 ${x2.toFixed(2)} ${y2.toFixed(2)}`
  }

  const trackPath = arcPath(TRACK_START, TRACK_START + 270, r)
  const valPath = value > 0.005 ? arcPath(TRACK_START, TRACK_START + value * 270, r) : null
  const needleDeg = TRACK_START + value * 270
  const [nx2, ny2] = pol(needleDeg, r * 0.6)
  const [nx1, ny1] = pol(needleDeg + 180, r * 0.12)

  const filterId = `glow-${id}`
  const gradId = `grad-${id}`
  const rimId = `rim-${id}`

  return (
    <svg
      width={size}
      height={size}
      style={{ cursor: 'ns-resize', userSelect: 'none', display: 'block' }}
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      onPointerUp={onPointerUp}
    >
      <defs>
        <filter id={filterId} x="-80%" y="-80%" width="260%" height="260%">
          <feGaussianBlur in="SourceGraphic" stdDeviation="5" result="blur" />
          <feMerge>
            <feMergeNode in="blur" />
            <feMergeNode in="SourceGraphic" />
          </feMerge>
        </filter>
        <radialGradient id={gradId} cx="38%" cy="30%" r="65%">
          <stop offset="0%" stopColor="#242e3e" />
          <stop offset="50%" stopColor="#131a26" />
          <stop offset="100%" stopColor="#080c14" />
        </radialGradient>
        <radialGradient id={rimId} cx="50%" cy="50%" r="50%">
          <stop offset="82%" stopColor="transparent" />
          <stop offset="100%" stopColor={`${glowColor}18`} />
        </radialGradient>
      </defs>

      {/* Outer glow ring */}
      <circle cx={cx} cy={cy} r={r + sw + 8} fill={`url(#${rimId})`} />

      {/* Outer bezel */}
      <circle cx={cx} cy={cy} r={r + sw + 5} fill="#080c14" stroke="#1c2840" strokeWidth={1.5} />

      {/* Inner bezel highlight */}
      <circle cx={cx} cy={cy} r={r + sw + 3.5} fill="none" stroke="rgba(255,255,255,0.03)" strokeWidth={1} />

      {/* Knob body */}
      <circle cx={cx} cy={cy} r={r + sw * 0.55} fill={`url(#${gradId})`} />

      {/* Specular highlight */}
      <ellipse cx={cx - r * 0.12} cy={cy - r * 0.2} rx={r * 0.4} ry={r * 0.25}
        fill="rgba(255,255,255,0.05)" />

      {/* Track */}
      {trackPath && (
        <path d={trackPath} fill="none" stroke="#121c2c" strokeWidth={sw} strokeLinecap="round" />
      )}

      {/* Value arc */}
      {valPath && (
        <path
          d={valPath}
          fill="none"
          stroke={accent}
          strokeWidth={sw}
          strokeLinecap="round"
          filter={`url(#${filterId})`}
          opacity={0.92}
        />
      )}

      {/* Needle */}
      <line
        x1={nx1} y1={ny1} x2={nx2} y2={ny2}
        stroke="rgba(220,235,255,0.95)"
        strokeWidth={Math.max(1.5, size * 0.014)}
        strokeLinecap="round"
      />

      {/* Center cap */}
      <circle cx={cx} cy={cy} r={r * 0.17} fill="#07090e" stroke={accent} strokeWidth={1.2} opacity={0.85} />
      <circle cx={cx} cy={cy} r={r * 0.08} fill={accent} opacity={0.5} />
    </svg>
  )
}

// ── Main App ───────────────────────────────────────────────────────────────
const WAVEFORMS = ['Sine', 'Square', 'Triangle', 'Sawtooth']

export default function App() {
  // freq: 0–1 (log scale 20 Hz – 2000 Hz)
  const [freq, setFreq] = useState(0.2)
  const [mix, setMix] = useState(0.5)
  const [waveform, setWaveform] = useState('Square')
  const [waveOpen, setWaveOpen] = useState(false)

  const freqHz = 20 * Math.pow(100, freq)
  const freqDisplay =
    freqHz >= 1000
      ? `${(freqHz / 1000).toFixed(2)} kHz`
      : `${freqHz.toFixed(2)} Hz`

  const mixPct = Math.round(mix * 100)

  return (
    <div
      style={{
        minHeight: '100vh',
        background: '#04060c',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        fontFamily: "'Rajdhani', sans-serif",
      }}
    >
      {/* ── Plugin shell ── */}
      <div
        style={{
          position: 'relative',
          width: 700,
          background: 'transparent',
          border: '1px solid #1e2e48',
          borderRadius: 6,
          boxShadow:
            '0 0 60px rgba(0,120,255,0.07), 0 0 120px rgba(0,60,255,0.04), 0 24px 80px rgba(0,0,0,0.9), inset 0 1px 0 rgba(255,255,255,0.05)',
          overflow: 'hidden',
        }}
      >
        {/* Layer 0 — Metallic texture, sits behind everything */}
        <MetallicBackground />

        {/* Layer 1 — All UI content, above the texture */}
        <div style={{ position: 'relative', zIndex: 1 }}>

        {/* ── Title bar ── */}
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            padding: '9px 16px',
            background: 'rgba(4,8,18,0.72)',
            borderBottom: '1px solid #182840',
          }}
        >
          <button
            style={{
              padding: '4px 16px',
              background: 'linear-gradient(180deg, #182a40 0%, #0e1e30 100%)',
              border: '1px solid #2a4560',
              borderRadius: 3,
              color: '#6090b0',
              fontSize: 11,
              fontFamily: 'inherit',
              fontWeight: 700,
              letterSpacing: '0.1em',
              cursor: 'pointer',
              textTransform: 'uppercase',
            }}
          >
            Options
          </button>

          <div style={{ flex: 1, textAlign: 'center' }}>
            <span
              style={{
                fontFamily: "'Orbitron', sans-serif",
                fontSize: 16,
                fontWeight: 600,
                letterSpacing: '0.2em',
                color: '#b8d4ee',
                textShadow:
                  '0 0 20px rgba(100,180,255,0.35), 0 0 40px rgba(80,160,255,0.15)',
              }}
            >
              RING MOD
            </span>
          </div>

          <div
            style={{
              fontSize: 12,
              color: '#284060',
              fontFamily: "'Orbitron', sans-serif",
              fontWeight: 700,
              letterSpacing: '0.12em',
            }}
          >
            JUCE
          </div>
        </div>

        {/* Subtitle strip */}
        <div
          style={{
            padding: '4px 16px',
            borderBottom: '1px solid #0e1b2c',
            display: 'flex',
            gap: 10,
            alignItems: 'center',
            background: 'rgba(3,6,14,0.60)',
          }}
        >
          <span
            style={{
              fontSize: 9,
              color: '#2e5070',
              letterSpacing: '0.14em',
              fontWeight: 700,
              textTransform: 'uppercase',
            }}
          >
            JUCE DSP
          </span>
          <span style={{ color: '#162030', fontSize: 10, fontWeight: 300 }}>│</span>
          <span
            style={{
              fontSize: 9,
              color: '#2e5070',
              letterSpacing: '0.14em',
              fontWeight: 700,
              textTransform: 'uppercase',
            }}
          >
            Phase Accumulator
          </span>
        </div>

        {/* ── Content row ── */}
        <div
          style={{
            display: 'flex',
            padding: '20px 24px 16px',
            gap: 16,
            alignItems: 'center',
          }}
        >
          {/* ── Left: Frequency ── */}
          <div
            style={{
              flex: 1,
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
              gap: 10,
            }}
          >
            {/* Freq display */}
            <div
              style={{
                width: '100%',
                background: '#020408',
                border: '1px solid #1a3850',
                borderRadius: 3,
                padding: '7px 12px 8px',
                boxShadow:
                  'inset 0 2px 10px rgba(0,0,0,0.7), 0 0 18px rgba(0,180,255,0.07)',
                position: 'relative',
              }}
            >
              <div
                style={{
                  position: 'absolute',
                  top: 5,
                  right: 10,
                  fontSize: 8,
                  color: '#1e4060',
                  fontFamily: "'Share Tech Mono', monospace",
                  letterSpacing: '0.05em',
                }}
              >
                280
              </div>
              <div
                style={{
                  fontSize: 26,
                  color: '#00c8ff',
                  fontFamily: "'Share Tech Mono', monospace",
                  textShadow:
                    '0 0 16px rgba(0,200,255,0.9), 0 0 40px rgba(0,200,255,0.4)',
                  textAlign: 'center',
                  letterSpacing: '0.03em',
                }}
              >
                {freqDisplay}
              </div>
            </div>

            <div
              style={{
                fontSize: 8,
                color: '#204050',
                letterSpacing: '0.08em',
                alignSelf: 'flex-start',
                fontFamily: "'Share Tech Mono', monospace",
              }}
            >
              Range: 20 Hz – 2000 Hz
            </div>

            <Knob value={freq} onChange={setFreq} accent="#00c8ff" glowColor="#00c8ff" size={152} id="freq" />

            <div
              style={{
                fontSize: 10,
                letterSpacing: '0.18em',
                color: '#3a6880',
                fontWeight: 700,
                textTransform: 'uppercase',
              }}
            >
              Carrier Frequency
            </div>

            <div
              style={{
                display: 'flex',
                justifyContent: 'space-between',
                width: '100%',
              }}
            >
              <span
                style={{
                  fontSize: 8,
                  color: '#1e4060',
                  fontFamily: "'Share Tech Mono', monospace",
                }}
              >
                20
              </span>
              <span
                style={{
                  fontSize: 8,
                  color: '#1e4060',
                  fontFamily: "'Share Tech Mono', monospace",
                }}
              >
                kHz
              </span>
            </div>
          </div>

          {/* ── Center: Waveform ── */}
          <div
            style={{
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
              gap: 10,
            }}
          >
            <div
              style={{
                fontSize: 8,
                color: '#284860',
                letterSpacing: '0.14em',
                fontWeight: 700,
                textTransform: 'uppercase',
                fontFamily: "'Share Tech Mono', monospace",
              }}
            >
              Carrier Buffer
            </div>

            {/* Waveform display — runtime image goes here */}
            <div
              style={{
                width: 168,
                height: 128,
                background: '#010408',
                border: '1px solid #182e40',
                borderRadius: 3,
                position: 'relative',
                overflow: 'hidden',
                boxShadow:
                  'inset 0 0 24px rgba(0,0,0,0.85), 0 0 20px rgba(0,150,255,0.05)',
              }}
            >
              {/* Grid overlay */}
              <svg
                width="100%"
                height="100%"
                style={{ position: 'absolute', inset: 0 }}
              >
                {[32, 64, 96].map((y) => (
                  <line
                    key={`h${y}`}
                    x1={0}
                    y1={y}
                    x2={168}
                    y2={y}
                    stroke="#0c1e2a"
                    strokeWidth={1}
                  />
                ))}
                {[42, 84, 126].map((x) => (
                  <line
                    key={`v${x}`}
                    x1={x}
                    y1={0}
                    x2={x}
                    y2={128}
                    stroke="#0c1e2a"
                    strokeWidth={1}
                  />
                ))}
                <line x1={0} y1={64} x2={168} y2={64} stroke="#162838" strokeWidth={1} />
              </svg>

              {/* Corner ticks */}
              {[
                [4, 4],
                [160, 4],
                [4, 120],
                [160, 120],
              ].map(([x, y], i) => (
                <div
                  key={i}
                  style={{
                    position: 'absolute',
                    left: x - 3,
                    top: y - 3,
                    width: 6,
                    height: 6,
                    border: '1px solid #1e4060',
                    borderRadius: 1,
                  }}
                />
              ))}

              {/* Runtime waveform renders here */}
              <div
                style={{
                  position: 'absolute',
                  inset: 0,
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <span
                  style={{
                    fontSize: 7,
                    color: '#162030',
                    letterSpacing: '0.12em',
                    fontFamily: "'Share Tech Mono', monospace",
                  }}
                >
                  WAVEFORM
                </span>
              </div>
            </div>

            {/* Waveform label */}
            <div
              style={{
                fontSize: 8,
                color: '#204050',
                letterSpacing: '0.1em',
                fontFamily: "'Share Tech Mono', monospace",
              }}
            >
              Waveform
            </div>

            {/* Waveform selector */}
            <div style={{ position: 'relative' }}>
              <button
                onClick={() => setWaveOpen((o) => !o)}
                style={{
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'space-between',
                  gap: 12,
                  width: 168,
                  padding: '8px 16px',
                  background:
                    'linear-gradient(180deg, #0c1e32 0%, #081628 100%)',
                  border: '1px solid #1e4a6a',
                  borderRadius: 3,
                  color: '#00c8ff',
                  fontSize: 14,
                  fontFamily: 'inherit',
                  fontWeight: 700,
                  letterSpacing: '0.1em',
                  cursor: 'pointer',
                  textShadow: '0 0 12px rgba(0,200,255,0.6)',
                  boxShadow:
                    '0 0 14px rgba(0,140,255,0.1), inset 0 1px 0 rgba(255,255,255,0.05)',
                  textTransform: 'uppercase',
                }}
              >
                <span>{waveform}</span>
                <span
                  style={{
                    fontSize: 9,
                    opacity: 0.7,
                    transform: waveOpen ? 'rotate(180deg)' : 'none',
                    transition: 'transform 0.2s',
                    display: 'inline-block',
                  }}
                >
                  ▼
                </span>
              </button>

              {waveOpen && (
                <div
                  style={{
                    position: 'absolute',
                    top: '100%',
                    left: 0,
                    right: 0,
                    zIndex: 100,
                    background: '#0a1828',
                    border: '1px solid #1e4a6a',
                    borderTop: 'none',
                    borderRadius: '0 0 3px 3px',
                    boxShadow: '0 8px 24px rgba(0,0,0,0.8)',
                  }}
                >
                  {WAVEFORMS.map((w) => (
                    <button
                      key={w}
                      onClick={() => {
                        setWaveform(w)
                        setWaveOpen(false)
                      }}
                      style={{
                        display: 'block',
                        width: '100%',
                        padding: '8px 16px',
                        background: w === waveform ? 'rgba(0,200,255,0.08)' : 'none',
                        border: 'none',
                        borderBottom: '1px solid #0e2030',
                        color: w === waveform ? '#00c8ff' : '#4a7090',
                        fontSize: 12,
                        fontFamily: 'inherit',
                        fontWeight: 700,
                        letterSpacing: '0.1em',
                        cursor: 'pointer',
                        textAlign: 'left',
                        textTransform: 'uppercase',
                      }}
                    >
                      {w}
                    </button>
                  ))}
                </div>
              )}
            </div>
          </div>

          {/* ── Right: Mix ── */}
          <div
            style={{
              flex: 1,
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
              gap: 10,
            }}
          >
            {/* Mix label + value */}
            <div
              style={{
                width: '100%',
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'baseline',
              }}
            >
              <span
                style={{
                  fontSize: 10,
                  letterSpacing: '0.18em',
                  color: '#2e7050',
                  fontWeight: 700,
                  textTransform: 'uppercase',
                }}
              >
                Mix:
              </span>
              <span
                style={{
                  fontSize: 22,
                  fontFamily: "'Share Tech Mono', monospace",
                  color: '#00ff88',
                  textShadow:
                    '0 0 16px rgba(0,255,136,0.7), 0 0 40px rgba(0,255,136,0.3)',
                }}
              >
                {mix.toFixed(2)}
              </span>
            </div>

            <div
              style={{
                fontSize: 8,
                color: '#235040',
                letterSpacing: '0.08em',
                alignSelf: 'flex-start',
                fontFamily: "'Share Tech Mono', monospace",
              }}
            >
              Value: {mix.toFixed(1)} ({mixPct}% Wet)
            </div>

            {/* Mix bar */}
            <div style={{ width: '100%' }}>
              <div
                style={{
                  display: 'flex',
                  justifyContent: 'space-between',
                  marginBottom: 5,
                }}
              >
                <span
                  style={{
                    fontSize: 7,
                    color: '#1a3830',
                    fontFamily: "'Share Tech Mono', monospace",
                  }}
                >
                  0%
                </span>
                <span
                  style={{
                    fontSize: 7,
                    color: '#1a3830',
                    fontFamily: "'Share Tech Mono', monospace",
                  }}
                >
                  100%
                </span>
              </div>
              <div
                style={{
                  height: 9,
                  background: '#050e08',
                  border: '1px solid #143828',
                  borderRadius: 2,
                  overflow: 'hidden',
                  cursor: 'pointer',
                }}
                onClick={(e) => {
                  const rect = e.currentTarget.getBoundingClientRect()
                  setMix(
                    Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width))
                  )
                }}
              >
                <div
                  style={{
                    height: '100%',
                    width: `${mixPct}%`,
                    background:
                      'linear-gradient(90deg, #00aa55 0%, #00ff88 100%)',
                    boxShadow: '0 0 10px rgba(0,255,136,0.5)',
                    borderRadius: 1,
                    transition: 'width 0.04s',
                  }}
                />
              </div>
            </div>

            <Knob value={mix} onChange={setMix} accent="#00ff88" glowColor="#00ff88" size={116} id="mix" />

            <div
              style={{
                fontSize: 10,
                letterSpacing: '0.18em',
                color: '#2e6050',
                fontWeight: 700,
                textTransform: 'uppercase',
              }}
            >
              Dry/Wet Mix
            </div>
          </div>
        </div>

        {/* ── Status bar ── */}
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            padding: '7px 16px',
            background: 'rgba(2,4,10,0.68)',
            borderTop: '1px solid #0e1b2c',
            gap: 24,
          }}
        >
          <span
            style={{
              fontSize: 8,
              color: '#1e3050',
              letterSpacing: '0.14em',
              fontWeight: 700,
              textTransform: 'uppercase',
              fontFamily: "'Share Tech Mono', monospace",
            }}
          >
            PARAMETERS
          </span>

          <div style={{ display: 'flex', gap: 20, alignItems: 'center' }}>
            <Pip color="#00c8ff" label="freq" value={freqHz.toFixed(1)} />
            <Pip color="#00ff88" label="mix" value={mix.toFixed(1)} />
          </div>

          <div
            style={{
              marginLeft: 'auto',
              fontSize: 8,
              color: '#1e3848',
              letterSpacing: '0.07em',
              fontFamily: "'Share Tech Mono', monospace",
            }}
          >
            Waveform: {waveform} | Phase Accumulator | carrierBuffer (Ready)
          </div>
        </div>

        </div>{/* end Layer 1 content */}

        {/* Layer 20 — Scanlines over everything */}
        <div
          style={{
            position: 'absolute',
            inset: 0,
            pointerEvents: 'none',
            backgroundImage:
              'repeating-linear-gradient(0deg, transparent, transparent 3px, rgba(0,0,0,0.10) 3px, rgba(0,0,0,0.10) 4px)',
            zIndex: 20,
          }}
        />

        {/* Layer 21 — Corner accent marks */}
        {[
          { top: 0, left: 0, borderTop: '1px solid', borderLeft: '1px solid' },
          { top: 0, right: 0, borderTop: '1px solid', borderRight: '1px solid' },
          { bottom: 0, left: 0, borderBottom: '1px solid', borderLeft: '1px solid' },
          { bottom: 0, right: 0, borderBottom: '1px solid', borderRight: '1px solid' },
        ].map((s, i) => (
          <div
            key={i}
            style={{
              position: 'absolute',
              width: 12,
              height: 12,
              borderColor: '#00c8ff55',
              ...s,
              zIndex: 21,
            }}
          />
        ))}

      </div>
    </div>
  )
}

function Pip({ color, label, value }: { color: string; label: string; value: string }) {
  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
      <div
        style={{
          width: 6,
          height: 6,
          borderRadius: '50%',
          background: color,
          boxShadow: `0 0 6px ${color}`,
        }}
      />
      <span
        style={{
          fontSize: 8,
          color: '#2a5060',
          letterSpacing: '0.06em',
          fontFamily: "'Share Tech Mono', monospace",
        }}
      >
        {label}{' '}
        <span style={{ color: '#3a7060' }}>{value}</span>
      </span>
    </div>
  )
}
