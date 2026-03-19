(function () {
  const canvas = document.getElementById("particleCanvas");
  if (!canvas) {
    return;
  }

  const ctx = canvas.getContext("2d", { alpha: true });
  if (!ctx) {
    return;
  }

  const css = getComputedStyle(document.documentElement);
  const config = {
    count: 60,
    sizeMin: 0.8,
    sizeMax: 1.6,
    speedMin: 5,
    speedMax: 25,
    turn: 1.8,
    glow: 10,
    color: css.getPropertyValue("--ea-secondary").trim() || "#58b8ff",
    background: css.getPropertyValue("--ea-bg").trim() || "#08131d",
    backgroundSoft: css.getPropertyValue("--ea-bg-soft").trim() || "#102131",
    primaryGlow: "rgba(246, 197, 73, 0.10)",
    secondaryGlow: "rgba(88, 184, 255, 0.12)",
    dprMax: 2,
  };

  let width = 0;
  let height = 0;
  let dpr = 1;
  let lastTs = 0;
  let running = true;
  const dots = [];

  function rand(min, max) {
    return min + Math.random() * (max - min);
  }

  function clamp(value, min, max) {
    return Math.min(max, Math.max(min, value));
  }

  function fit() {
    const cssWidth = canvas.clientWidth || window.innerWidth;
    const cssHeight = canvas.clientHeight || window.innerHeight;
    dpr = Math.max(1, Math.min(window.devicePixelRatio || 1, config.dprMax));

    canvas.width = cssWidth * dpr;
    canvas.height = cssHeight * dpr;
    width = canvas.width / dpr;
    height = canvas.height / dpr;

    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.scale(dpr, dpr);
  }

  function makeDot() {
    const speed = rand(config.speedMin, config.speedMax);
    const angle = rand(0, Math.PI * 2);
    return {
      x: rand(0, width),
      y: rand(0, height),
      vx: Math.cos(angle) * speed,
      vy: Math.sin(angle) * speed,
      r: rand(config.sizeMin, config.sizeMax),
    };
  }

  function initDots() {
    dots.length = 0;
    for (let index = 0; index < config.count; index += 1) {
      dots.push(makeDot());
    }
  }

  function stepDot(dot, dt) {
    const angle = Math.atan2(dot.vy, dot.vx);
    const delta = (Math.random() - 0.5) * config.turn * dt;
    const nextAngle = angle + delta;
    const speed = clamp(Math.hypot(dot.vx, dot.vy) || config.speedMin, config.speedMin, config.speedMax);

    dot.vx = Math.cos(nextAngle) * speed;
    dot.vy = Math.sin(nextAngle) * speed;
    dot.x += dot.vx * dt;
    dot.y += dot.vy * dt;

    if (dot.x < -20) {
      dot.x = width + 20;
    } else if (dot.x > width + 20) {
      dot.x = -20;
    }

    if (dot.y < -20) {
      dot.y = height + 20;
    } else if (dot.y > height + 20) {
      dot.y = -20;
    }
  }

  function drawDot(dot) {
    ctx.beginPath();
    ctx.arc(dot.x, dot.y, dot.r, 0, Math.PI * 2);
    ctx.shadowBlur = config.glow;
    ctx.shadowColor = config.color;
    ctx.fillStyle = config.color;
    ctx.fill();
  }

  function frame(ts) {
    if (!running) {
      return;
    }

    if (!lastTs) {
      lastTs = ts;
    }

    const dt = Math.min(0.05, (ts - lastTs) / 1000);
    lastTs = ts;

    ctx.globalCompositeOperation = "source-over";
    const backgroundGradient = ctx.createLinearGradient(0, 0, 0, height);
    backgroundGradient.addColorStop(0, config.backgroundSoft);
    backgroundGradient.addColorStop(1, config.background);
    ctx.fillStyle = backgroundGradient;
    ctx.fillRect(0, 0, width, height);

    const primaryGlow = ctx.createRadialGradient(width * 0.16, height * 0.1, 0, width * 0.16, height * 0.1, Math.max(width, height) * 0.34);
    primaryGlow.addColorStop(0, config.primaryGlow);
    primaryGlow.addColorStop(1, "rgba(246, 197, 73, 0)");
    ctx.fillStyle = primaryGlow;
    ctx.fillRect(0, 0, width, height);

    const secondaryGlow = ctx.createRadialGradient(width * 0.78, height * 0.82, 0, width * 0.78, height * 0.82, Math.max(width, height) * 0.42);
    secondaryGlow.addColorStop(0, config.secondaryGlow);
    secondaryGlow.addColorStop(1, "rgba(88, 184, 255, 0)");
    ctx.fillStyle = secondaryGlow;
    ctx.fillRect(0, 0, width, height);

    dots.forEach((dot) => {
      stepDot(dot, dt);
      drawDot(dot);
    });

    window.requestAnimationFrame(frame);
  }

  function onVisibilityChange() {
    running = document.visibilityState !== "hidden";
    if (running) {
      lastTs = 0;
      window.requestAnimationFrame(frame);
    }
  }

  window.addEventListener("resize", () => {
    fit();
    dots.forEach((dot) => {
      dot.x = clamp(dot.x, -20, width + 20);
      dot.y = clamp(dot.y, -20, height + 20);
    });
  });

  document.addEventListener("visibilitychange", onVisibilityChange);

  fit();
  initDots();
  window.requestAnimationFrame(frame);
})();
