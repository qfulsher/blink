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

// --- Upload ---

const fileInput = document.getElementById("file");
const uploadBtn = document.getElementById("upload");
const uploadStatus = document.getElementById("upload-status");

function fmtBytes(n) {
    if (n < 1024) return `${n} B`;
    if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KB`;
    return `${(n / 1024 / 1024).toFixed(2)} MB`;
}

fileInput.addEventListener("change", () => {
    uploadBtn.disabled = fileInput.files.length === 0;
    uploadStatus.textContent = "";
});

uploadBtn.addEventListener("click", () => {
    const file = fileInput.files[0];
    if (!file) return;

    uploadBtn.disabled = true;
    uploadStatus.textContent = `Uploading ${file.name} (${fmtBytes(file.size)})…`;

    // XHR instead of fetch so we get progress events.
    const xhr = new XMLHttpRequest();
    xhr.open("POST", `/api/files/${encodeURIComponent(file.name)}`);
    xhr.setRequestHeader("Content-Type", "application/octet-stream");

    xhr.upload.addEventListener("progress", (e) => {
        if (!e.lengthComputable) return;
        const pct = ((e.loaded / e.total) * 100).toFixed(0);
        uploadStatus.textContent = `Uploading ${file.name}: ${pct}% (${fmtBytes(e.loaded)} / ${fmtBytes(e.total)})`;
    });

    xhr.addEventListener("load", () => {
        if (xhr.status >= 200 && xhr.status < 300) {
            try {
                const body = JSON.parse(xhr.responseText);
                uploadStatus.textContent = `Uploaded ${fmtBytes(body.size)} → /${body.path}`;
            } catch {
                uploadStatus.textContent = `Uploaded (HTTP ${xhr.status})`;
            }
            fileInput.value = "";
        } else {
            uploadStatus.textContent = `Error: HTTP ${xhr.status} — ${xhr.responseText || xhr.statusText}`;
        }
        uploadBtn.disabled = fileInput.files.length === 0;
    });

    xhr.addEventListener("error", () => {
        uploadStatus.textContent = "Network error during upload";
        uploadBtn.disabled = fileInput.files.length === 0;
    });

    xhr.send(file);
});
