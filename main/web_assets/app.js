const stateEl = document.getElementById("state");
const toggleBtn = document.getElementById("toggle");

let currentOn = null;

function render(on) {
    currentOn = on;
    stateEl.classList.remove("state-unknown", "state-on", "state-off");
    stateEl.classList.add(on ? "state-on" : "state-off");
    stateEl.textContent = on ? "ON" : "OFF";
    toggleBtn.disabled = false;
}

async function fetchState() {
    const res = await fetch("/api/led");
    if (!res.ok) throw new Error(`GET /api/led failed: ${res.status}`);
    const body = await res.json();
    render(body.on);
}

async function setState(on) {
    toggleBtn.disabled = true;
    const res = await fetch("/api/led", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ on }),
    });
    if (!res.ok) throw new Error(`POST /api/led failed: ${res.status}`);
    const body = await res.json();
    render(body.on);
}

toggleBtn.addEventListener("click", () => {
    if (currentOn === null) return;
    setState(!currentOn).catch(console.error);
});

fetchState().catch(console.error);
