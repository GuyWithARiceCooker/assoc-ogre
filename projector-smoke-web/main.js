// Mobile-first visible demo. Canvas2D is used intentionally here: it is stable on
// phones, and every visible fog sample is computed from the same projector-light
// equation. There are no separate "intersection blob" objects.

const canvas = document.createElement('canvas');
const ctx = canvas.getContext('2d', { alpha: false });
document.body.prepend(canvas);

let mode = 0;
let orbit = true;
let quality = matchMedia('(pointer: coarse), (max-width: 720px)').matches ? 0 : 1;
let time = 0;
let last = performance.now();
let cameraAngle = -0.55;
let cells = [];
let sparseRays = [];

const smokeCenter = { x: 0, y: 16, z: 0 };
const projectors = [
  { pos: { x: -78, y: 38, z: 70 }, color: [255, 48, 40], id: 0 },
  { pos: { x: 78, y: 34, z: 66 }, color: [50, 255, 120], id: 1 },
  { pos: { x: -72, y: 30, z: -78 }, color: [63, 116, 255], id: 2 },
  { pos: { x: 82, y: 44, z: -70 }, color: [255, 220, 66], id: 3 },
];

function resize() {
  const ratio = quality === 0 ? 0.72 : quality === 1 ? 0.92 : 1.15;
  const dpr = Math.min(devicePixelRatio || 1, ratio);
  canvas.width = Math.max(240, Math.floor(innerWidth * dpr));
  canvas.height = Math.max(320, Math.floor(innerHeight * dpr));
  canvas.style.width = '100vw';
  canvas.style.height = '100vh';
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}
resize();
addEventListener('resize', () => { resize(); rebuildField(); });

function pattern(u, v, id) {
  const r = Math.hypot(u, v);
  const a = Math.atan2(v, u);
  if (id === 0) return Math.max(softBand(r, 0.52, 0.09), softBand(u - v, 0, 0.095), softBand(u + v, 0, 0.095));
  if (id === 1) {
    const cross = Math.max(softBand(u, 0, 0.17), softBand(v, 0, 0.17));
    const blocks = smoothStep(0.45, 0.54, Math.abs(u)) * smoothStep(0.45, 0.54, Math.abs(v)) * (1 - smoothStep(0.84, 0.96, Math.max(Math.abs(u), Math.abs(v))));
    return Math.max(cross, blocks);
  }
  if (id === 2) return smoothStep(0.70, 0.95, Math.abs(Math.sin(5.0 * a + 8.0 * r))) * (1 - smoothStep(0.82, 0.98, r));
  const checker = ((Math.floor((u + 1) * 4) + Math.floor((v + 1) * 4)) % 2) === 0 ? 0.78 : 0.0;
  return Math.max(checker * (1 - smoothStep(0.86, 1.0, Math.max(Math.abs(u), Math.abs(v)))), 1 - smoothStep(0.25, 0.34, r));
}

function smoothStep(a, b, x) {
  const t = Math.max(0, Math.min(1, (x - a) / (b - a)));
  return t * t * (3 - 2 * t);
}
function softBand(x, center, width) {
  return 1 - smoothStep(width * 0.55, width, Math.abs(x - center));
}
function sub(a, b) { return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }; }
function add(a, b) { return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }; }
function mul(a, s) { return { x: a.x * s, y: a.y * s, z: a.z * s }; }
function dot(a, b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
function cross(a, b) { return { x: a.y*b.z - a.z*b.y, y: a.z*b.x - a.x*b.z, z: a.x*b.y - a.y*b.x }; }
function norm(a) { const l = Math.hypot(a.x, a.y, a.z) || 1; return mul(a, 1 / l); }
function clamp255(x) { return Math.max(0, Math.min(255, Math.round(x))); }

function basis(forward) {
  const f = norm(forward);
  const upGuess = Math.abs(dot(f, { x: 0, y: 1, z: 0 })) > 0.92 ? { x: 1, y: 0, z: 0 } : { x: 0, y: 1, z: 0 };
  const right = norm(cross(f, upGuess));
  const up = norm(cross(right, f));
  return { right, up, forward: f };
}

const projectorFrames = projectors.map((proj) => {
  const forward = norm(sub(smokeCenter, proj.pos));
  return { ...proj, ...basis(forward), lens: add(proj.pos, mul(forward, 7)) };
});

function rotateY(p, a) {
  const s = Math.sin(a), c = Math.cos(a);
  return { x: p.x * c - p.z * s, y: p.y, z: p.x * s + p.z * c };
}
function project(p) {
  const w = innerWidth, h = innerHeight;
  const cam = rotateY(p, -cameraAngle);
  const z = cam.z + 135;
  const f = Math.min(w, h) * 1.15;
  return { x: w * 0.5 + cam.x * f / z, y: h * 0.54 - (cam.y - 10) * f / z, z };
}
function rgba(c, a) { return `rgba(${clamp255(c[0])},${clamp255(c[1])},${clamp255(c[2])},${a})`; }

function densityAt(p) {
  const qx = (p.x - smokeCenter.x) / 62;
  const qy = (p.y - smokeCenter.y) / 36;
  const qz = (p.z - smokeCenter.z) / 62;
  const ellipsoid = 1 - smoothStep(0.72, 1.08, qx*qx + qy*qy + qz*qz);
  const wisp = 0.72 + 0.28 * Math.sin(p.x * 0.09 + p.z * 0.07 + p.y * 0.11 + time * 0.45);
  return Math.max(0, ellipsoid * wisp);
}

function projectorLightAt(p) {
  const rgb = [0, 0, 0];
  let hits = 0;
  const contributors = [];
  for (const proj of projectorFrames) {
    const d = sub(p, proj.pos);
    const z = dot(d, proj.forward);
    if (z <= 2) continue;
    const fovScale = 0.42;
    const u = dot(d, proj.right) / (z * fovScale);
    const v = dot(d, proj.up) / (z * fovScale);
    if (Math.abs(u) > 1 || Math.abs(v) > 1) continue;
    const img = pattern(u, v, proj.id);
    if (img <= 0.01) continue;
    const edge = 1 - smoothStep(0.78, 1.0, Math.max(Math.abs(u), Math.abs(v)));
    const distAtten = 1 / (1 + 0.00085 * dot(d, d));
    const contrib = img * edge * distAtten * 5.8;
    rgb[0] += proj.color[0] * contrib;
    rgb[1] += proj.color[1] * contrib;
    rgb[2] += proj.color[2] * contrib;
    hits += Math.min(1, contrib * 0.55);
    contributors.push({ proj, contrib });
  }
  return { rgb, hits, contributors };
}

function rebuildField() {
  const grid = quality === 0 ? 30 : quality === 1 ? 40 : 52;
  const yLayers = quality === 0 ? [10, 18, 26] : [6, 14, 22, 30];
  cells = [];
  sparseRays = [];
  for (const y of yLayers) {
    for (let iz = 0; iz < grid; iz++) {
      for (let ix = 0; ix < grid; ix++) {
        const x = -64 + (ix / (grid - 1)) * 128;
        const z = -64 + (iz / (grid - 1)) * 128;
        const p = { x, y, z };
        const dens = densityAt(p);
        if (dens < 0.025) continue;
        const light = projectorLightAt(p);
        const energy = (light.rgb[0] + light.rgb[1] + light.rgb[2]) / 765;
        if (energy < 0.012 && dens < 0.22) continue;
        cells.push({ p, dens, ...light, energy });
        if (energy > 0.10 && sparseRays.length < 130) {
          for (const c of light.contributors) {
            if (c.contrib > 0.10 && (ix + iz + y) % 3 === 0) sparseRays.push({ from: c.proj.lens, to: p, color: c.proj.color, contrib: c.contrib });
          }
        }
      }
    }
  }
}
rebuildField();

function drawLine3(a, b, color, alpha, width = 1) {
  const pa = project(a), pb = project(b);
  if (pa.z <= 1 || pb.z <= 1) return;
  ctx.strokeStyle = rgba(color, alpha);
  ctx.lineWidth = width;
  ctx.beginPath(); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); ctx.stroke();
}
function drawCell(cell) {
  const pp = project(cell.p);
  if (pp.z <= 1) return;
  let rgb = cell.rgb;
  let alpha = Math.min(0.58, 0.035 + cell.energy * 0.46 + cell.dens * 0.045);
  if (mode === 1) {
    const multi = smoothStep(0.85, 2.2, cell.hits);
    rgb = [rgb[0] * 0.25 + 255 * multi, rgb[1] * 0.23 + 220 * multi, rgb[2] * 0.18 + 70 * multi];
    alpha = Math.min(0.86, alpha + multi * 0.32);
  } else if (mode === 2) {
    alpha *= 0.48;
  }
  const radius = (quality === 0 ? 2.2 : 1.75) * Math.min(innerWidth, innerHeight) / pp.z;
  const grad = ctx.createRadialGradient(pp.x, pp.y, 0, pp.x, pp.y, radius * 2.7);
  grad.addColorStop(0, rgba(rgb, alpha));
  grad.addColorStop(1, rgba(rgb, 0));
  ctx.fillStyle = grad;
  ctx.beginPath(); ctx.arc(pp.x, pp.y, radius * 2.7, 0, Math.PI * 2); ctx.fill();
}

function drawProjectorIcons() {
  for (const proj of projectorFrames) {
    const pp = project(proj.pos);
    if (pp.z <= 1) continue;
    const r = 5 * Math.min(innerWidth, innerHeight) / pp.z;
    ctx.fillStyle = rgba(proj.color, 0.90);
    ctx.beginPath(); ctx.arc(pp.x, pp.y, Math.max(3, r * 2.5), 0, Math.PI * 2); ctx.fill();
  }
}

function drawGrid() {
  for (let i = -9; i <= 9; i++) {
    drawLine3({ x: i * 10, y: 0, z: -90 }, { x: i * 10, y: 0, z: 90 }, [80,120,160], 0.16);
    drawLine3({ x: -90, y: 0, z: i * 10 }, { x: 90, y: 0, z: i * 10 }, [80,120,160], 0.16);
  }
}

function render() {
  const w = innerWidth, h = innerHeight;
  ctx.clearRect(0, 0, w, h);
  const bg = ctx.createLinearGradient(0, 0, 0, h);
  bg.addColorStop(0, '#0b1020'); bg.addColorStop(0.55, '#050912'); bg.addColorStop(1, '#02040a');
  ctx.fillStyle = bg; ctx.fillRect(0, 0, w, h);
  drawGrid();

  if (mode === 2) {
    for (const ray of sparseRays) drawLine3(ray.from, ray.to, ray.color, Math.min(0.36, 0.08 + ray.contrib * 0.08), 1.15);
  } else {
    for (const ray of sparseRays.slice(0, 40)) drawLine3(ray.from, ray.to, ray.color, 0.035, 0.8);
  }

  // Back-to-front helps the volumetric field look less like separate sprites.
  const sorted = cells.slice().sort((a, b) => project(b.p).z - project(a.p).z);
  for (const cell of sorted) drawCell(cell);
  drawProjectorIcons();

  ctx.fillStyle = 'rgba(235,245,255,0.78)';
  ctx.font = '12px system-ui, sans-serif';
  ctx.fillText(mode === 1 ? 'szamolt osszeg: azonos fustpontban tobb projektor' : mode === 2 ? 'utvonalak, amelyek a fustmintakat megvilagitjak' : 'minden lathato pont szamolt fenyosszeg', 14, h - 70);
}

function animate(now = performance.now()) {
  requestAnimationFrame(animate);
  if (document.hidden) return;
  const dt = Math.min(0.033, (now - last) / 1000 || 0.016);
  last = now;
  time += dt;
  if (orbit) cameraAngle += dt * 0.22;
  // Rebuild occasionally because density is animated.
  if (Math.floor(time * 8) !== Math.floor((time - dt) * 8)) rebuildField();
  render();
}

function setMode(next) {
  mode = next;
  document.querySelectorAll('[data-mode]').forEach(btn => btn.classList.toggle('active', Number(btn.dataset.mode) === mode));
}
function setQuality(next) {
  quality = next;
  document.getElementById('qualityBtn').textContent = ['Low', 'Med', 'High'][quality];
  document.getElementById('qualityBtn').classList.toggle('active', quality > 0);
  resize(); rebuildField();
}

document.querySelectorAll('[data-mode]').forEach(btn => btn.addEventListener('click', () => setMode(Number(btn.dataset.mode))));
document.getElementById('orbitBtn').addEventListener('click', () => {
  orbit = !orbit;
  document.getElementById('orbitBtn').classList.toggle('active', orbit);
});
document.getElementById('qualityBtn').addEventListener('click', () => setQuality((quality + 1) % 3));
addEventListener('keydown', e => { if (e.key === '1') setMode(0); if (e.key === '2') setMode(1); if (e.key === '3') setMode(2); if (e.key === ' ') orbit = !orbit; });

setMode(0); setQuality(quality); animate();
