// Mobile-first visible demo. The smoke is now a live particle volume: particles
// drift, swirl, respawn, and at their current position receive light from all
// four projectors. Bright regions are true per-smoke-particle light sums.

const canvas = document.createElement('canvas');
const ctx = canvas.getContext('2d', { alpha: false });
document.body.prepend(canvas);

let mode = 0;
let orbit = true;
let quality = matchMedia('(pointer: coarse), (max-width: 720px)').matches ? 0 : 1;
let time = 0;
let last = performance.now();
let cameraAngle = -0.22;
let smoke = [];
let frameNo = 0;

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
addEventListener('resize', () => { resize(); seedSmoke(); });

function smoothStep(a, b, x) { const t = Math.max(0, Math.min(1, (x - a) / (b - a))); return t * t * (3 - 2 * t); }
function softBand(x, center, width) { return 1 - smoothStep(width * 0.55, width, Math.abs(x - center)); }
function sub(a, b) { return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }; }
function add(a, b) { return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }; }
function mul(a, s) { return { x: a.x * s, y: a.y * s, z: a.z * s }; }
function dot(a, b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
function cross(a, b) { return { x: a.y*b.z - a.z*b.y, y: a.z*b.x - a.x*b.z, z: a.x*b.y - a.y*b.x }; }
function norm(a) { const l = Math.hypot(a.x, a.y, a.z) || 1; return mul(a, 1 / l); }
function clamp255(x) { return Math.max(0, Math.min(255, Math.round(x))); }
function rgba(c, a) { return `rgba(${clamp255(c[0])},${clamp255(c[1])},${clamp255(c[2])},${a})`; }

function pattern(u, v, id) {
  const r = Math.hypot(u, v);
  const a = Math.atan2(v, u);
  if (id === 0) return Math.max(softBand(r, 0.52, 0.09), softBand(u - v, 0, 0.095), softBand(u + v, 0, 0.095));
  if (id === 1) {
    const crossShape = Math.max(softBand(u, 0, 0.17), softBand(v, 0, 0.17));
    const blocks = smoothStep(0.45, 0.54, Math.abs(u)) * smoothStep(0.45, 0.54, Math.abs(v)) * (1 - smoothStep(0.84, 0.96, Math.max(Math.abs(u), Math.abs(v))));
    return Math.max(crossShape, blocks);
  }
  if (id === 2) return smoothStep(0.70, 0.95, Math.abs(Math.sin(5.0 * a + 8.0 * r))) * (1 - smoothStep(0.82, 0.98, r));
  const checker = ((Math.floor((u + 1) * 4) + Math.floor((v + 1) * 4)) % 2) === 0 ? 0.78 : 0.0;
  return Math.max(checker * (1 - smoothStep(0.86, 1.0, Math.max(Math.abs(u), Math.abs(v)))), 1 - smoothStep(0.25, 0.34, r));
}

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

function rotateY(p, a) { const s = Math.sin(a), c = Math.cos(a); return { x: p.x * c - p.z * s, y: p.y, z: p.x * s + p.z * c }; }
function project(p) {
  const w = innerWidth, h = innerHeight;
  const cam = rotateY(p, -cameraAngle);
  const z = cam.z + 135;
  const f = Math.min(w, h) * 1.15;
  return { x: w * 0.5 + cam.x * f / z, y: h * 0.54 - (cam.y - 10) * f / z, z };
}

function randn() { return Math.random() * 2 - 1; }
function randomSmokePoint() {
  // Smoke is emitted from a smoke machine/nozzle at the lower-left/front side,
  // then it drifts into the invisible projection volume.
  return {
    x: -34 + randn() * 5,
    y: 3 + Math.random() * 4,
    z: 28 + randn() * 6
  };
}
function seedSmoke() {
  const count = quality === 0 ? 520 : quality === 1 ? 820 : 1200;
  smoke = [];
  for (let i = 0; i < count; i++) {
    const p = randomSmokePoint();
    const warm = Math.random();
    p.x += warm * 46 + randn() * (2 + warm * 18);
    p.y += warm * 28 + randn() * (1 + warm * 12);
    p.z -= warm * 38 + randn() * (2 + warm * 18);
    smoke.push({
      p,
      age: warm * 5,
      life: 5 + Math.random() * 8,
      seed: Math.random() * 1000,
      size: 0.62 + Math.random() * 1.05,
      density: 0.25 + Math.random() * 0.55,
    });
  }
}
seedSmoke();

function flowVelocity(p, seed, tt) {
  // Plume flow: smoke machine pushes toward the projection volume, then
  // buoyancy and curl spread it into a living cloud.
  const cx = p.x * 0.035, cy = p.y * 0.045, cz = p.z * 0.035;
  const target = { x: 6, y: 20, z: -4 };
  const toCenter = norm(sub(target, p));
  const a = Math.sin(cz * 2.1 + tt * 0.7 + seed) + Math.cos(cy * 1.3 - tt * 0.45);
  const b = Math.cos(cx * 1.8 - tt * 0.55 + seed * 0.7) + Math.sin(cz * 1.2 + tt * 0.35);
  return {
    x: toCenter.x * 2.0 + (-p.z * 0.010 + a * 0.9),
    y: 0.95 + toCenter.y * 1.0 + Math.sin(cx + cz + tt) * 0.30,
    z: toCenter.z * 2.0 + (p.x * 0.010 + b * 0.9)
  };
}
function updateSmoke(dt) {
  for (const s of smoke) {
    s.age += dt;
    const v = flowVelocity(s.p, s.seed, time);
    s.p.x += v.x * dt * 5.0;
    s.p.y += v.y * dt * 4.0;
    s.p.z += v.z * dt * 5.0;
    // Soft containment. Escaped/old particles respawn at the bottom-ish of the cloud.
    const qx = s.p.x / 82, qy = (s.p.y - 18) / 48, qz = s.p.z / 82;
    if (qx*qx + qy*qy + qz*qz > 1.65 || s.p.y > 58 || s.age > s.life) {
      s.p = randomSmokePoint();
      s.age = 0;
      s.life = 5 + Math.random() * 8;
      s.seed = Math.random() * 1000;
    }
  }
}

function particleDensity(s) {
  const qx = s.p.x / 78, qy = (s.p.y - 18) / 45, qz = s.p.z / 78;
  const ellipsoid = 1 - smoothStep(0.92, 1.35, qx*qx + qy*qy + qz*qz);
  const fade = smoothStep(0.0, 0.8, s.age) * (1 - smoothStep(s.life - 1.6, s.life, s.age));
  const wisp = 0.75 + 0.25 * Math.sin(s.p.x * 0.08 + s.p.z * 0.06 + s.seed + time * 0.8);
  return Math.max(0, ellipsoid * fade * wisp * s.density * 0.62);
}


function sculptureField(p) {
  // Moving sculpture: a twisting double-ribbon/helix in the smoke.
  // We compute distance to animated 3D curves; particles near those curves catch light.
  const q = { x: p.x - 5, y: p.y - 22, z: p.z + 2 };
  let best = 1e9;
  for (let i = 0; i < 28; i++) {
    const t = -3.14159 + (i / 27) * 6.28318;
    const spin = time * 0.85;
    const y = (t / 3.14159) * 24.0;
    const r = 15.0 + 3.0 * Math.sin(t * 3.0 + spin);
    const c1 = {
      x: Math.sin(t + spin) * r,
      y,
      z: Math.cos(t * 1.35 + spin * 0.8) * 11.0
    };
    const c2 = {
      x: Math.sin(t + spin + 3.14159) * (r * 0.72),
      y: y + Math.sin(t * 2.0 + spin) * 2.0,
      z: Math.cos(t * 1.35 + spin * 0.8 + 3.14159) * 9.0
    };
    const d1 = Math.hypot(q.x - c1.x, q.y - c1.y, q.z - c1.z) - 4.2;
    const d2 = Math.hypot(q.x - c2.x, q.y - c2.y, q.z - c2.z) - 3.2;
    best = Math.min(best, d1, d2);
  }
  // Small orbiting knots make it feel sculptural rather than a single line.
  for (let k = 0; k < 3; k++) {
    const a = time * 0.9 + k * 2.094;
    const c = {
      x: Math.cos(a) * 18.0,
      y: Math.sin(a * 1.7) * 12.0,
      z: Math.sin(a) * 14.0
    };
    best = Math.min(best, Math.hypot(q.x - c.x, q.y - c.y, q.z - c.z) - 5.0);
  }
  return best;
}

function sculptureGlow(p) {
  const d = sculptureField(p);
  // Bright only near the moving surface; soft halo in smoke, not empty air.
  return Math.max(0, 1 - smoothStep(0.0, 5.2, Math.abs(d)));
}

function approximateNormal(p) {
  const e = 1.8;
  return norm({
    x: sculptureField({ x: p.x + e, y: p.y, z: p.z }) - sculptureField({ x: p.x - e, y: p.y, z: p.z }),
    y: sculptureField({ x: p.x, y: p.y + e, z: p.z }) - sculptureField({ x: p.x, y: p.y - e, z: p.z }),
    z: sculptureField({ x: p.x, y: p.y, z: p.z + e }) - sculptureField({ x: p.x, y: p.y, z: p.z - e })
  });
}

function projectedSculptureImage(u, v, id, p) {
  // Each projector carries a view-dependent image of the same moving sculpture.
  // The image is not a flat texture on a plane: it gates light by the 3D sculpture field.
  const sculpture = sculptureGlow(p);
  if (sculpture <= 0.0) return 0;
  const stripe = 0.65 + 0.35 * Math.sin((u * 9.0 + v * 5.5) + time * (1.1 + id * 0.17));
  const contour = 0.55 + 0.45 * Math.sin((u * u + v * v) * 18.0 - time * 1.6 + id);
  return sculpture * (0.55 + 0.30 * stripe + 0.15 * contour);
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
    const img = projectedSculptureImage(u, v, proj.id, p);
    if (img <= 0.001) continue;
    const edge = 1 - smoothStep(0.80, 1.0, Math.max(Math.abs(u), Math.abs(v)));
    const distAtten = 1 / (1 + 0.00115 * dot(d, d));
    const normal = approximateNormal(p);
    const incoming = norm(sub(proj.pos, p));
    const phase = 0.35 + 0.65 * Math.pow(Math.max(0, dot(normal, incoming)) * 0.65 + 0.35, 1.7);
    const contrib = img * edge * distAtten * phase * 13.0;
    // Slightly desaturate as real smoke scatters coloured projector light.
    const whiteScatter = contrib * 32;
    rgb[0] += proj.color[0] * contrib + whiteScatter;
    rgb[1] += proj.color[1] * contrib + whiteScatter;
    rgb[2] += proj.color[2] * contrib + whiteScatter;
    hits += Math.min(1, contrib * 0.60);
    contributors.push({ proj, contrib });
  }
  return { rgb, hits, contributors, energy: (rgb[0] + rgb[1] + rgb[2]) / 765 };
}

function drawLine3(a, b, color, alpha, width = 1) {
  const pa = project(a), pb = project(b);
  if (pa.z <= 1 || pb.z <= 1) return;
  ctx.strokeStyle = rgba(color, alpha);
  ctx.lineWidth = width;
  ctx.beginPath(); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); ctx.stroke();
}
function drawParticle(s) {
  const dens = particleDensity(s);
  if (dens <= 0.01) return;
  const light = projectorLightAt(s.p);
  const pp = project(s.p);
  if (pp.z <= 1) return;

  if (light.energy <= 0.0035) return;
  let rgb = light.rgb;
  let alpha = Math.min(0.62, dens * Math.pow(light.energy, 0.68) * 0.86);
  if (mode === 1) {
    const multi = smoothStep(0.8, 2.15, light.hits);
    rgb = [rgb[0] * 0.28 + 255 * multi, rgb[1] * 0.25 + 225 * multi, rgb[2] * 0.20 + 70 * multi];
    alpha = Math.min(0.50, alpha + multi * 0.18);
  } else if (mode === 2) {
    alpha = Math.min(0.42, alpha * 1.7);
  }

  const r = s.size * (quality === 0 ? 2.1 : 1.65) * Math.min(innerWidth, innerHeight) / pp.z;
  const g = ctx.createRadialGradient(pp.x, pp.y, 0, pp.x, pp.y, r * 2.4);
  g.addColorStop(0, rgba(rgb, alpha));
  g.addColorStop(0.55, rgba(rgb, alpha * 0.22));
  g.addColorStop(1, rgba(rgb, 0));
  ctx.fillStyle = g;
  ctx.beginPath(); ctx.arc(pp.x, pp.y, r * 2.4, 0, Math.PI * 2); ctx.fill();

}

function drawSmokeMachine() {
  // The only visible hardware: a small smoke machine/nozzle feeding the plume.
  const base = project({ x: -36, y: 1.5, z: 30 });
  const nozzle = project({ x: -28, y: 5.5, z: 24 });
  if (base.z <= 1 || nozzle.z <= 1) return;
  ctx.save();
  ctx.fillStyle = 'rgba(22,28,38,0.96)';
  ctx.strokeStyle = 'rgba(120,150,180,0.25)';
  ctx.lineWidth = 1;
  const w = Math.max(34, 2600 / base.z);
  const h = Math.max(16, 1100 / base.z);
  ctx.beginPath();
  ctx.roundRect(base.x - w * 0.5, base.y - h * 0.5, w, h, 4);
  ctx.fill(); ctx.stroke();
  ctx.strokeStyle = 'rgba(180,210,230,0.78)';
  ctx.lineWidth = Math.max(2, 160 / base.z);
  ctx.beginPath(); ctx.moveTo(base.x + w * 0.25, base.y - h * 0.25); ctx.lineTo(nozzle.x, nozzle.y); ctx.stroke();
  ctx.restore();
}

function drawProjectorHardware() {
  const c = project(smokeCenter);
  const screenW = Math.min(390, canvas.clientWidth || innerWidth, document.documentElement.clientWidth || innerWidth);
  const screenH = canvas.clientHeight || document.documentElement.clientHeight || innerHeight;
  const anchors = [
    { x: 45, y: 145 },
    { x: screenW - 45, y: 145 },
    { x: 45, y: Math.max(275, screenH * 0.43) },
    { x: screenW - 45, y: Math.max(275, screenH * 0.43) },
  ];

  for (let i = 0; i < projectorFrames.length; i++) {
    const proj = projectorFrames[i];
    const a = anchors[i];
    const angle = Math.atan2(c.y - a.y, c.x - a.x);
    const scale = Math.max(0.9, Math.min(1.18, innerWidth / 390));
    const bodyW = 38 * scale;
    const bodyH = 22 * scale;

    ctx.save();
    ctx.translate(a.x, a.y);
    ctx.rotate(angle);
    ctx.fillStyle = 'rgba(24,32,44,0.98)';
    ctx.strokeStyle = 'rgba(185,210,240,0.62)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.roundRect(-bodyW * 0.55, -bodyH * 0.5, bodyW, bodyH, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = rgba(proj.color, 0.96);
    ctx.beginPath();
    ctx.arc(bodyW * 0.50, 0, Math.max(4.4, 6.2 * scale), 0, Math.PI * 2);
    ctx.fill();

    ctx.strokeStyle = 'rgba(220,235,255,0.24)';
    ctx.beginPath();
    ctx.moveTo(-bodyW * 0.25, -bodyH * 0.5);
    ctx.lineTo(-bodyW * 0.42, -bodyH * 1.02);
    ctx.lineTo(bodyW * 0.05, -bodyH * 0.5);
    ctx.stroke();
    ctx.restore();

    // Always-visible label/lens marker for mobile readability.
    ctx.save();
    ctx.fillStyle = rgba(proj.color, 1.0);
    ctx.beginPath();
    ctx.arc(a.x, a.y, 5.5 * scale, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = 'rgba(230,240,255,0.86)';
    ctx.font = `${Math.round(10 * scale)}px system-ui, sans-serif`;
    ctx.fillText(`P${i + 1}`, a.x - 9 * scale, a.y - 11 * scale);
    ctx.restore();
  }
}

function render() {
  const w = innerWidth, h = innerHeight;
  ctx.clearRect(0, 0, w, h);
  const bg = ctx.createLinearGradient(0, 0, 0, h);
  bg.addColorStop(0, '#0b1020'); bg.addColorStop(0.55, '#050912'); bg.addColorStop(1, '#02040a');
  ctx.fillStyle = bg; ctx.fillRect(0, 0, w, h);
  drawSmokeMachine();
  const sorted = smoke.slice().sort((a, b) => project(b.p).z - project(a.p).z);
  for (const s of sorted) drawParticle(s);
  drawProjectorHardware();
  ctx.fillStyle = 'rgba(235,245,255,0.82)';
  ctx.font = '12px system-ui, sans-serif';
  ctx.fillText(mode === 1 ? 'a mozgó szobor tobbszorosen megvilagitott fustpontjai' : mode === 2 ? 'a vetitett szobor csak a fustben jelenik meg' : 'mozgó 3D szobor a fustben', 14, h - 70);
}

function animate(now = performance.now()) {
  requestAnimationFrame(animate);
  if (document.hidden) return;
  const dt = Math.min(0.033, (now - last) / 1000 || 0.016);
  last = now;
  time += dt;
  frameNo++;
  if (orbit) cameraAngle += dt * 0.20;
  updateSmoke(dt);
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
  resize(); seedSmoke();
}

document.querySelectorAll('[data-mode]').forEach(btn => btn.addEventListener('click', () => setMode(Number(btn.dataset.mode))));
document.getElementById('orbitBtn').addEventListener('click', () => { orbit = !orbit; document.getElementById('orbitBtn').classList.toggle('active', orbit); });
document.getElementById('qualityBtn').addEventListener('click', () => setQuality((quality + 1) % 3));
addEventListener('keydown', e => { if (e.key === '1') setMode(0); if (e.key === '2') setMode(1); if (e.key === '3') setMode(2); if (e.key === ' ') orbit = !orbit; });

setMode(0); setQuality(quality); animate();
