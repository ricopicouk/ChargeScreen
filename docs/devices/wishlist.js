(() => {
  const support = window.CHARGESCREEN_SUPPORT || {};
  const list = document.getElementById("wishlist-results");
  const requestForm = document.querySelector("[data-wishlist-form]");
  const message = document.getElementById("wishlist-form-message");
  const fallbackEmail = support.submission?.fallbackEmail || "contact@chargescreen.co.uk";
  const escapeHtml = (value) =>
    String(value)
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");

  if (list) {
    list.innerHTML = (support.wishlist || []).map((item) => `
      <article class="support-card">
        <div class="support-card-top">
          <span class="status-pill">${escapeHtml(item.status)}</span>
        </div>
        <h3>${escapeHtml(item.name)}</h3>
        <p>${escapeHtml(item.note)}</p>
      </article>
    `).join("");
  }

  if (requestForm && support.submission?.enabled && support.submission?.endpoint) {
    requestForm.action = support.submission.endpoint;
    requestForm.method = "post";
    if (message) message.textContent = "Requests are reviewed before anything appears on the website.";
    return;
  }

  if (message) {
    message.innerHTML = `Online wishlist submissions are not connected yet. This button opens an email to <a href="mailto:${fallbackEmail}">${fallbackEmail}</a>.`;
  }

  requestForm?.addEventListener("submit", (event) => {
    event.preventDefault();
    const data = new FormData(requestForm);
    const subject = `ChargeScreen wishlist request: ${data.get("device") || "New device"}`;
    const body = [
      `Device: ${data.get("device") || ""}`,
      `App: ${data.get("app") || ""}`,
      `Name: ${data.get("name") || ""}`,
      `Email: ${data.get("email") || ""}`,
      "",
      "Details:",
      data.get("details") || ""
    ].join("\n");
    window.location.href = `mailto:${fallbackEmail}?subject=${encodeURIComponent(subject)}&body=${encodeURIComponent(body)}`;
  });
})();
