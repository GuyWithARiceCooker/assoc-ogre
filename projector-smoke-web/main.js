// Mobile-first visible demo. Uses Canvas2D by default because mobile WebGL/raymarching
// can silently fail or render too dark on some browsers. The drawing still models the
// same idea: four full-image projector fields are evaluated in one shared fog volume.

const canvas = document.createElement('canvas');
const ctx = canvas.getContext('2d', { alpha: false });
document.body.prepend(canvas);

let mode = 0;
let orbit = true;
let quality = matchMedia('(pointer: coarse), (max-width: 720px)').matches ? 0 : 1;
let time = 0;
let last = performance.now();
let cameraAngle = -0.55;

const smokeCenter = { x: 0, y: 16, z: 0 };
const projectors = [
  { pos: { x: -78, y: 38, z: 70 }, color: [255, 48, 40], id: 0 },
  { pos: { x: 78, y: 34, z: 66 }, color: [50, 255, 120], id: 1 },
  { pos: { x: -72, y: 30, z: -78 }, color: [63, 116, 255], id: 2 },
  { pos: { x: 82, y: 44, z: -70 }, color: [255, 220, 66], id: 3 },
];

function resize() {
  const ratio = quality === 0 ? 0.7 : quality === 1 ? 0.9 : 1.15;
  const dpr = Math.min(devicePixelRatio || 1, ratio);
  canvas.width = Math.max(240, Math.floor(innerWidth * dpr));
  canvas.height = Math.max(320, Math.floor(innerHeight * dpr));
  canvas.style.width = '100vw';
  canvas.style.height = '100vh';
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}
resize();
addEventListener('resize', resize);

function pattern(u, v, id) {
  const r = Math.hypot(u, v);
  const a = Math.atan2(v, u);
  if (id === 0) return Math.abs(r - 0.52) < 0.085 || Math.abs(u - v) < 0.09 || Math.abs(u + v) < 0.09;
  if (id === 1) return Math.abs(u) < 0.16 || Math.abs(v) < 0.16 || (Math.abs(u) > 0.45 && Math.abs(v) > 0.45 && Math.abs(u) < 0.84 && Math.abs(v) < 0.84);
  if (id === 2) return Math.abs(Math.sin(5.0 * a + 8.0 * r)) > 0.72 && r < 0.95;
  const checker = ((Math.floor((u + 1) * 4) + Math.floor((v + 1) * 4)) % 2) === 0;
  return (checker && Math.abs(u) < 0.9 && Math.abs(v) < 0.9) || r < 0.30;
}

function sub(a, b) { return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }; }
function add(a, b) { return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }; }
function mul(a, s) { return { x: a.x * s, y: a.y * s, z: a.z * s }; }
function dot(a, b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
function cross(a, b) { return { x: a.y*b.z - a.z*b.y, y: a.z*b.x - a.x*b.z, z: a.x*b.y - a.y*b.x }; }
function norm(a) { const l = Math.hypot(a.x, a.y, a.z) || 1; return mul(a, 1 / l); }

function basis(forward) {
  const f = norm(forward);
  const upGuess = Math.abs(dot(f, { x: 0, y: 1, z: 0 })) > 0.92 ? { x: 1, y: 0, z: 0 } : { x: 0, y: 1, z: 0 };
  const right = norm(cross(f, upGuess));
  const up = norm(cross(right, f));
  return { right, up, forward: f };
}

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

function rgba(c, a) { return `rgba(${c[0]},${c[1]},${c[2]},${a})`; }

function drawLine3(a, b, color, alpha, width = 1) {
  const pa = project(a), pb = project(b);
  if (pa.z <= 1 || pb.z <= 1) return;
  ctx.strokeStyle = rgba(color, alpha);
  ctx.lineWidth = width;
  ctx.beginPath(); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); ctx.stroke();
}

function drawDot3(p, color, alpha, radius) {
  const pp = project(p);
  if (pp.z <= 1) return;
  const r = radius * Math.min(innerWidth, innerHeight) / pp.z;
  const g = ctx.createRadialGradient(pp.x, pp.y, 0, pp.x, pp.y, r * 4);
  g.addColorStop(0, rgba(color, alpha));
  g.addColorStop(1, rgba(color, 0));
  ctx.fillStyle = g;
  ctx.beginPath(); ctx.arc(pp.x, pp.y, r * 4, 0, Math.PI * 2); ctx.fill();
}

function projectorSamples(proj) {
  const fwd = norm(sub(smokeCenter, proj.pos));
  const b = basis(fwd);
  const lens = add(proj.pos, mul(fwd, 7));
  const planeCenter = add(smokeCenter, mul(fwd, proj.id % 2 ? 1.8 : -1.8));
  const planeSize = 44;
  const grid = quality === 0 ? 15 : quality === 1 ? 19 : 23;
  const out = [];
  for (let y = 0; y < grid; y++) {
    for (let x = 0; x < grid; x++) {
      const u = (x / (grid - 1)) * 2 - 1;
      const v = (y / (grid - 1)) * 2 - 1;
      if (!pattern(u, v, proj.id)) continue;
      const target = add(add(planeCenter, mul(b.right, u * planeSize * 0.5)), mul(b.up, v * planeSize * 0.5));
      out.push({ lens, target, u, v });
    }
  }
  return out;
}

let sampleCache = [];
function rebuildSamples() { sampleCache = projectors.map(projectorSamples); }
rebuildSamples();

function drawFog() {
  const w = innerWidth, h = innerHeight;
  const center = project(smokeCenter);
  const rx = Math.min(w, h) * 0.34;
  const ry = Math.min(w, h) * 0.24;
  for (let i = 0; i < 26; i++) {
    const a = i * 2.399 + time * 0.13;
    const rr = ((i * 37) % 100) / 100;
    const x = center.x + Math.cos(a) * rx * rr;
    const y = center.y + Math.sin(a * 1.7) * ry * rr;
    const r = (28 + (i % 7) * 10) * (quality === 0 ? 0.8 : 1.0);
    const g = ctx.createRadialGradient(x, y, 0, x, y, r);
    g.addColorStop(0, 'rgba(120,145,180,0.050)');
    g.addColorStop(1, 'rgba(120,145,180,0)');
    ctx.fillStyle = g;
    ctx.beginPath(); ctx.arc(x, y, r, 0, Math.PI * 2); ctx.fill();
  }
}

function render() {
  const w = innerWidth, h = innerHeight;
  ctx.clearRect(0, 0, w, h);
  const bg = ctx.createLinearGradient(0, 0, 0, h);
  bg.addColorStop(0, '#0b1020'); bg.addColorStop(0.55, '#050912'); bg.addColorStop(1, '#02040a');
  ctx.fillStyle = bg; ctx.fillRect(0, 0, w, h);

  // Ground grid.
  ctx.strokeStyle = 'rgba(80,120,160,0.18)'; ctx.lineWidth = 1;
  for (let i = -9; i <= 9; i++) {
    drawLine3({ x: i * 10, y: 0, z: -90 }, { x: i * 10, y: 0, z: 90 }, [80,120,160], 0.18);
    drawLine3({ x: -90, y: 0, z: i * 10 }, { x: 90, y: 0, z: i * 10 }, [80,120,160], 0.18);
  }

  drawFog();

  // Full image projection: each bright pattern pixel contributes both along the path and at the shared fog volume.
  for (let pi = 0; pi < projectors.length; pi++) {
    const proj = projectors[pi];
    const samples = sampleCache[pi];
    const rayAlpha = mode === 1 ? 0.025 : mode === 2 ? 0.26 : 0.11;
    const dotAlpha = mode === 1 ? 0.78 : mode === 2 ? 0.22 : 0.52;
    for (const s of samples) {
      drawLine3(s.lens, s.target, proj.color, rayAlpha, mode === 2 ? 1.35 : 0.85);
    }
    for (const s of samples) {
      drawDot3(s.target, proj.color, dotAlpha, mode === 1 ? 2.25 : 1.4);
    }
    drawDot3(proj.pos, proj.color, 0.8, 2.7);
  }

  const c = project(smokeCenter);
  const pulse = 1 + Math.sin(time * 2.8) * 0.12;
  const halo = ctx.createRadialGradient(c.x, c.y, 0, c.x, c.y, Math.min(w, h) * 0.24 * pulse);
  halo.addColorStop(0, mode === 1 ? 'rgba(255,238,160,0.52)' : 'rgba(255,238,180,0.30)');
  halo.addColorStop(0.28, 'rgba(255,210,120,0.14)');
  halo.addColorStop(1, 'rgba(255,210,120,0)');
  ctx.fillStyle = halo;
  ctx.beginPath(); ctx.arc(c.x, c.y, Math.min(w, h) * 0.24 * pulse, 0, Math.PI * 2); ctx.fill();

  ctx.fillStyle = 'rgba(235,245,255,0.78)';
  ctx.font = '12px system-ui, sans-serif';
  ctx.fillText(mode === 1 ? 'osszeadott kozos fustterfogat' : mode === 2 ? 'sugarutak / mellektermek' : 'komplett projektorkepek a fustben', 14, h - 70);
}

function animate(now = performance.now()) {
  requestAnimationFrame(animate);
  if (document.hidden) return;
  const dt = Math.min(0.033, (now - last) / 1000 || 0.016);
  last = now;
  time += dt;
  if (orbit) cameraAngle += dt * 0.32;
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
  resize(); rebuildSamples();
}

document.querySelectorAll('[data-mode]').forEach(btn => btn.addEventListener('click', () => setMode(Number(btn.dataset.mode))));
document.getElementById('orbitBtn').addEventListener('click', () => {
  orbit = !orbit;
  document.getElementById('orbitBtn').classList.toggle('active', orbit);
});
document.getElementById('qualityBtn').addEventListener('click', () => setQuality((quality + 1) % 3));
addEventListener('keydown', e => { if (e.key === '1') setMode(0); if (e.key === '2') setMode(1); if (e.key === '3') setMode(2); if (e.key === ' ') orbit = !orbit; });

setMode(0); setQuality(quality); animate();
