(() => {
  const support = window.CHARGESCREEN_SUPPORT || {};
  const devices = support.devices || [];
  const list = document.getElementById("device-results");
  const filter = document.getElementById("device-filter");
  const status = document.getElementById("device-count");
  const escapeHtml = (value) =>
    String(value)
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");

  const createDeviceCard = (device) => {
    const card = document.createElement("article");
    card.className = "support-card";
    card.innerHTML = `
      <div class="support-card-top">
        <span class="status-pill">${escapeHtml(device.status)}</span>
        <span class="device-type">${escapeHtml(device.type)}</span>
      </div>
      <h3><a href="${encodeURIComponent(device.slug)}/">${escapeHtml(device.name)}</a></h3>
      <p>${escapeHtml(device.key)}</p>
      <ul>${device.shows.map((item) => `<li>${escapeHtml(item)}</li>`).join("")}</ul>
      <a class="text-link" href="${encodeURIComponent(device.slug)}/">Setup notes</a>
    `;
    return card;
  };

  const render = () => {
    const query = (filter?.value || "").trim().toLowerCase();
    const matches = devices.filter((device) => {
      const text = [device.name, device.type, device.status, device.key, ...device.shows].join(" ").toLowerCase();
      return !query || text.includes(query);
    });

    if (status) {
      status.textContent = `${matches.length} of ${devices.length} devices shown`;
    }

    if (!list) return;
    list.innerHTML = "";
    matches.forEach((device) => list.appendChild(createDeviceCard(device)));
  };

  filter?.addEventListener("input", render);
  render();
})();
