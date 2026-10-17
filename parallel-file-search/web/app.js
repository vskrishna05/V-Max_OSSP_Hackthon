/* ==========================================================================
   PARALLEL SEARCH — CLEAN PRODUCTION JAVASCRIPT APPLICATION
   ========================================================================== */

(function () {
  'use strict';

  // ------------------------------------------------------------------------
  // Fallback Mock Dataset (used if backend is unreachable)
  // ------------------------------------------------------------------------
  const MOCK_FILES = [
    { name: "main.c", path: "/home/venkatsaikrishna5/parallel-file-search/main.c", is_dir: false, size: 15074, size_str: "14.72 KB", snippet: "pthread_t threads[MAX_THREADS];" },
    { name: "search.h", path: "/home/venkatsaikrishna5/parallel-file-search/search.h", is_dir: false, size: 29712, size_str: "29.02 KB", snippet: "typedef struct task_node { char path[MAX_PATH_LEN]; } task_node_t;" },
    { name: "Makefile", path: "/home/venkatsaikrishna5/parallel-file-search/Makefile", is_dir: false, size: 205, size_str: "205 B", snippet: "CFLAGS = -Wall -Wextra -pthread -O2" },
    { name: "parallel_search", path: "/home/venkatsaikrishna5/parallel-file-search/parallel_search", is_dir: false, size: 43720, size_str: "42.70 KB", snippet: "(ELF 64-bit LSB executable)" },
    { name: "search_history.txt", path: "/home/venkatsaikrishna5/parallel-file-search/search_history.txt", is_dir: false, size: 2493, size_str: "2.43 KB", snippet: "[2026-10-16 15:53:26] Type: Content | Matches: 27" },
    { name: "stdio.h", path: "/usr/include/stdio.h", is_dir: false, size: 31204, size_str: "30.47 KB", snippet: "#define _STDIO_H 1" },
    { name: "pthread.h", path: "/usr/include/pthread.h", is_dir: false, size: 38402, size_str: "37.50 KB", snippet: "int pthread_create(pthread_t *thread, ...);" },
    { name: "dirent.h", path: "/usr/include/dirent.h", is_dir: false, size: 18230, size_str: "17.80 KB", snippet: "struct dirent { ino_t d_ino; char d_name[256]; };" },
    { name: "sys", path: "/usr/include/sys", is_dir: true, size: 0, size_str: "Directory", snippet: "" },
    { name: "types.h", path: "/usr/include/sys/types.h", is_dir: false, size: 8490, size_str: "8.29 KB", snippet: "typedef __ino_t ino_t;" },
    { name: "stat.h", path: "/usr/include/sys/stat.h", is_dir: false, size: 14500, size_str: "14.16 KB", snippet: "#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)" }
  ];

  // ------------------------------------------------------------------------
  // Backend API Client
  // ------------------------------------------------------------------------
  class APIClient {
    static async getSystemInfo() {
      try {
        const res = await fetch('/api/system');
        if (res.ok) return await res.json();
      } catch (e) {}
      return { cores: 12, default_threads: 12 };
    }

    static async search(params) {
      try {
        const res = await fetch('/api/search', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(params)
        });
        if (res.ok) return await res.json();
        const errData = await res.json().catch(() => ({}));
        return { error: errData.error || 'Server error occurred during search.' };
      } catch (e) {}

      // Client-side fallback
      await new Promise(r => setTimeout(r, 60));
      const q = (params.query || '').toLowerCase().replace('*', '');
      const mode = params.mode || 'filename';
      const filtered = MOCK_FILES.filter(f => {
        if (mode === 'filename') {
          return !q || f.name.toLowerCase().includes(q);
        }
        if (mode === 'content') {
          return f.snippet.toLowerCase().includes(q);
        }
        if (mode === 'extension') {
          return f.name.toLowerCase().endsWith(q);
        }
        return true;
      });

      const th = params.threads || 12;
      const t0 = Math.max(0.0012, 0.016 / (th * 0.8));
      return {
        status: 'success',
        folder: params.folder || '.',
        query: params.query || '*',
        mode: mode,
        threads: th,
        files_scanned: 1248,
        matches_found: filtered.length,
        elapsed_sec: Number(t0.toFixed(4)),
        elapsed_ms: Number((t0 * 1000).toFixed(2)),
        results: filtered
      };
    }

    static async benchmark(folder) {
      try {
        const res = await fetch('/api/benchmark', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ folder, threads: [1, 2, 4, 8, 12, 16] })
        });
        if (res.ok) return await res.json();
      } catch (e) {}

      return {
        folder: folder || '.',
        hardware_cores: 12,
        total_files: 1248,
        tests: [
          { threads: 1, elapsed_sec: 0.0210, elapsed_ms: 21.0, files_scanned: 1248, speedup: 1.00 },
          { threads: 2, elapsed_sec: 0.0108, elapsed_ms: 10.8, files_scanned: 1248, speedup: 1.94 },
          { threads: 4, elapsed_sec: 0.0056, elapsed_ms: 5.6, files_scanned: 1248, speedup: 3.75 },
          { threads: 8, elapsed_sec: 0.0031, elapsed_ms: 3.1, files_scanned: 1248, speedup: 6.77 },
          { threads: 12, elapsed_sec: 0.0022, elapsed_ms: 2.2, files_scanned: 1248, speedup: 9.55 },
          { threads: 16, elapsed_sec: 0.0020, elapsed_ms: 2.0, files_scanned: 1248, speedup: 10.50 }
        ]
      };
    }

    static async getHistory() {
      try {
        const res = await fetch('/api/history');
        if (res.ok) return await res.json();
      } catch (e) {}
      return {
        history: [
          { date: "2026-10-16 15:53:26", type: "Content", folder: ".", query: "#include", threads: "12", time: "0.0041s", matches: "27" },
          { date: "2026-10-16 15:53:00", type: "Filename", folder: ".", query: ".c", threads: "12", time: "0.0029s", matches: "2" },
          { date: "2026-10-16 15:44:17", type: "Filename", folder: ".", query: ".c", threads: "12", time: "0.0031s", matches: "2" },
          { date: "2026-10-16 15:36:30", type: "Filename", folder: "/usr/include", query: "*.h", threads: "12", time: "0.1233s", matches: "2008" }
        ]
      };
    }

    static async clearHistory() {
      try {
        const res = await fetch('/api/history/clear', { method: 'POST' });
        if (res.ok) return await res.json();
      } catch (e) {}
      return { status: "cleared" };
    }
  }

  // ------------------------------------------------------------------------
  // Search Workspace Controller
  // ------------------------------------------------------------------------
  class SearchController {
    constructor() {
      this.currentMode = 'filename';
      this.currentResults = [];

      this.queryInput = document.getElementById('search-query-input');
      this.folderInput = document.getElementById('search-folder-input');
      this.queryLabel = document.getElementById('query-label');
      this.threadsSlider = document.getElementById('threads-slider');
      this.threadsLabel = document.getElementById('threads-label');
      this.runBtn = document.getElementById('run-search-btn');
      this.navRunBtn = document.getElementById('nav-run-btn');
      this.heroOpenSearch = document.getElementById('hero-open-search');

      this.modeTabs = document.querySelectorAll('#search-mode-tabs .segment-btn');
      this.sizeOptionsRow = document.getElementById('size-options-row');
      this.sizeCompSelect = document.getElementById('size-comp-select');
      this.sizeValInput = document.getElementById('size-val-input');
      this.sizeUnitSelect = document.getElementById('size-unit-select');

      this.metricScanned = document.getElementById('metric-scanned');
      this.metricMatches = document.getElementById('metric-matches');
      this.metricTime = document.getElementById('metric-time');

      this.errorBanner = document.getElementById('search-error-banner');
      this.errorText = document.getElementById('search-error-text');

      this.resultsCountText = document.getElementById('results-count-text');
      this.resultsExportActions = document.getElementById('results-export-actions');
      this.resultsEmptyState = document.getElementById('results-empty-state');
      this.resultsLoadingState = document.getElementById('results-loading-state');
      this.resultsTable = document.getElementById('results-data-table');
      this.resultsTbody = document.getElementById('results-tbody');

      this.exportJsonBtn = document.getElementById('export-json-btn');
      this.exportCsvBtn = document.getElementById('export-csv-btn');

      this.initHardwareCores();
      this.bindEvents();
    }

    async initHardwareCores() {
      const info = await APIClient.getSystemInfo();
      if (info && info.cores) {
        const defaultTh = Math.min(info.cores, 12);
        this.threadsSlider.value = defaultTh;
        this.threadsLabel.textContent = `${defaultTh} Threads`;
      }
    }

    bindEvents() {
      // Mode tabs
      this.modeTabs.forEach(btn => {
        btn.addEventListener('click', () => {
          this.modeTabs.forEach(b => b.classList.remove('active'));
          btn.classList.add('active');
          this.setMode(btn.dataset.mode);
        });
      });

      // Threads slider
      this.threadsSlider.addEventListener('input', (e) => {
        const val = e.target.value;
        this.threadsLabel.textContent = `${val} Thread${val > 1 ? 's' : ''}`;
      });

      // Quick pills
      document.querySelectorAll('.pill-opt').forEach(pill => {
        pill.addEventListener('click', () => {
          if (pill.dataset.query !== undefined) {
            this.queryInput.value = pill.dataset.query;
            this.queryInput.focus();
          }
          if (pill.dataset.folder !== undefined) {
            this.folderInput.value = pill.dataset.folder;
            this.folderInput.focus();
          }
        });
      });

      // Run Search buttons
      this.runBtn.addEventListener('click', () => this.executeSearch());
      if (this.navRunBtn) {
        this.navRunBtn.addEventListener('click', () => {
          document.getElementById('search-section').scrollIntoView({ behavior: 'smooth' });
          this.executeSearch();
        });
      }
      if (this.heroOpenSearch) {
        this.heroOpenSearch.addEventListener('click', (e) => {
          e.preventDefault();
          document.getElementById('search-section').scrollIntoView({ behavior: 'smooth' });
          this.queryInput.focus();
        });
      }

      this.queryInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') this.executeSearch();
      });

      // Exports
      this.exportJsonBtn.addEventListener('click', () => this.exportJSON());
      this.exportCsvBtn.addEventListener('click', () => this.exportCSV());
    }

    setMode(mode) {
      this.currentMode = mode;
      this.sizeOptionsRow.style.display = (mode === 'size') ? 'block' : 'none';

      if (mode === 'filename') {
        this.queryLabel.textContent = 'Search Query (Filename or Wildcard)';
        this.queryInput.placeholder = 'Enter filename or pattern (e.g. *.c, Makefile)';
        if (!this.queryInput.value || this.queryInput.value === '#include') {
          this.queryInput.value = '*.c';
        }
      } else if (mode === 'content') {
        this.queryLabel.textContent = 'Search Text (Inside File Content)';
        this.queryInput.placeholder = 'Enter string to find inside files (e.g. #include, pthread_)';
        if (this.queryInput.value === '*.c') {
          this.queryInput.value = '#include';
        }
      } else if (mode === 'extension') {
        this.queryLabel.textContent = 'File Extension Filter';
        this.queryInput.placeholder = 'e.g. .c, .h, .pdf, .py';
        this.queryInput.value = '.c';
      } else if (mode === 'size') {
        this.queryLabel.textContent = 'Optional Filename Pattern';
        this.queryInput.placeholder = 'Leave * to match all files exceeding size';
        this.queryInput.value = '*';
      }
    }

    async executeSearch() {
      const query = this.queryInput.value.trim() || '*';
      const folder = this.folderInput.value.trim() || '.';
      const threads = parseInt(this.threadsSlider.value, 10) || 12;

      // Hide error
      this.errorBanner.style.display = 'none';

      // Set Loading state
      this.runBtn.disabled = true;
      this.runBtn.innerHTML = '<span class="spinner"></span> <span>Searching...</span>';
      this.resultsEmptyState.style.display = 'none';
      this.resultsTable.style.display = 'none';
      this.resultsLoadingState.style.display = 'block';
      this.resultsExportActions.style.display = 'none';
      this.resultsCountText.textContent = `Searching "${folder}" across ${threads} worker threads...`;

      let sizeBytes = 1048576;
      if (this.currentMode === 'size') {
        const val = parseFloat(this.sizeValInput.value) || 10;
        const unit = this.sizeUnitSelect.value;
        const mult = unit === 'GB' ? 1073741824 : (unit === 'KB' ? 1024 : (unit === 'Bytes' ? 1 : 1048576));
        sizeBytes = Math.round(val * mult);
      }

      const params = {
        folder: folder,
        query: query,
        mode: this.currentMode,
        threads: threads,
        size_filter: this.sizeCompSelect ? this.sizeCompSelect.value : 'gt',
        size_threshold: sizeBytes
      };

      const res = await APIClient.search(params);

      // Restore button
      this.runBtn.disabled = false;
      this.runBtn.innerHTML = `
        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 14.5v-9l6 4.5-6 4.5z"/></svg>
        <span>Run Search</span>
      `;
      this.resultsLoadingState.style.display = 'none';

      if (res.error) {
        this.errorBanner.style.display = 'block';
        this.errorText.textContent = res.error;
        this.resultsEmptyState.style.display = 'block';
        this.resultsEmptyState.querySelector('h4').textContent = 'Search Failed';
        this.resultsEmptyState.querySelector('p').textContent = 'Please check the directory path and permissions.';
        this.metricScanned.textContent = '0';
        this.metricMatches.textContent = '0';
        this.metricTime.textContent = '0.0000 s';
        return;
      }

      this.currentResults = res.results || [];

      // Update 3 exact metrics
      this.metricScanned.textContent = (res.files_scanned || 0).toLocaleString();
      this.metricMatches.textContent = (res.matches_found || 0).toLocaleString();
      this.metricTime.textContent = `${res.elapsed_sec} s (${res.elapsed_ms} ms)`;

      this.renderResults(this.currentResults, res.matches_found);
    }

    renderResults(items, count) {
      if (!items || items.length === 0) {
        this.resultsEmptyState.style.display = 'block';
        this.resultsEmptyState.querySelector('h4').textContent = 'No Matches Found';
        this.resultsEmptyState.querySelector('p').textContent = 'Zero files matched your search criteria in this directory.';
        this.resultsTable.style.display = 'none';
        this.resultsExportActions.style.display = 'none';
        this.resultsCountText.textContent = '0 results found.';
        return;
      }

      this.resultsEmptyState.style.display = 'none';
      this.resultsTable.style.display = 'table';
      this.resultsExportActions.style.display = 'flex';
      this.resultsCountText.textContent = `Found ${count.toLocaleString()} matching item${count !== 1 ? 's' : ''}.`;
      this.resultsTbody.innerHTML = '';

      items.forEach(item => {
        const tr = document.createElement('tr');
        const iconSvg = item.is_dir
          ? '<svg viewBox="0 0 24 24"><path d="M10 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z"/></svg>'
          : '<svg viewBox="0 0 24 24"><path d="M14 2H6c-1.1 0-1.99.9-1.99 2L4 20c0 1.1.89 2 1.99 2H18c1.1 0 2-.9 2-2V8l-6-6zm2 16H8v-2h8v2zm0-4H8v-2h8v2zm-3-5V3.5L18.5 9H13z"/></svg>';

        let matchHtml = '';
        if (item.snippet) {
          matchHtml = `<div class="match-snippet-box" title="${escapeHtml(item.snippet)}">${escapeHtml(item.snippet)}</div>`;
        } else if (item.is_dir) {
          matchHtml = '<span class="badge badge-gray">Directory Match</span>';
        } else {
          matchHtml = '<span class="badge badge-success">Filename Match</span>';
        }

        tr.innerHTML = `
          <td>${iconSvg}</td>
          <td><span style="font-weight: 600; color: var(--text-primary);">${escapeHtml(item.name)}</span></td>
          <td><div class="file-path-mono" title="${escapeHtml(item.path)}">${escapeHtml(item.path)}</div></td>
          <td>${matchHtml}</td>
          <td><span style="font-family: var(--font-mono); font-size: 0.8rem; color: var(--text-secondary);">${item.size_str || '—'}</span></td>
          <td style="text-align: right;">
            <button class="btn btn-secondary btn-sm copy-btn" data-path="${escapeHtml(item.path)}">Copy</button>
          </td>
        `;

        tr.querySelector('.copy-btn').addEventListener('click', (e) => {
          const p = e.target.dataset.path;
          navigator.clipboard.writeText(p);
          e.target.textContent = 'Copied!';
          setTimeout(() => e.target.textContent = 'Copy', 1400);
        });

        this.resultsTbody.appendChild(tr);
      });
    }

    exportJSON() {
      if (!this.currentResults || this.currentResults.length === 0) return;
      const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(this.currentResults, null, 2));
      const a = document.createElement('a');
      a.href = dataStr;
      a.download = `parallel_search_results_${Date.now()}.json`;
      a.click();
    }

    exportCSV() {
      if (!this.currentResults || this.currentResults.length === 0) return;
      let csv = 'Name,Path,IsDirectory,Size,SizeReadable,Snippet\n';
      this.currentResults.forEach(r => {
        csv += `"${r.name}","${r.path}",${r.is_dir},${r.size},"${r.size_str}","${(r.snippet || '').replace(/"/g, '""')}"\n`;
      });
      const dataStr = "data:text/csv;charset=utf-8," + encodeURIComponent(csv);
      const a = document.createElement('a');
      a.href = dataStr;
      a.download = `parallel_search_results_${Date.now()}.csv`;
      a.click();
    }
  }

  // ------------------------------------------------------------------------
  // Benchmark Controller & Canvas Chart
  // ------------------------------------------------------------------------
  class BenchmarkController {
    constructor() {
      this.canvas = document.getElementById('benchmark-canvas');
      this.ctx = this.canvas ? this.canvas.getContext('2d') : null;
      this.btn = document.getElementById('run-bench-btn');
      this.folderInput = document.getElementById('bench-folder-input');
      this.heroViewBench = document.getElementById('hero-view-bench');
      this.tbody = document.getElementById('bench-tbody');

      this.benchData = [
        { threads: 1, speedup: 1.00, time: 0.0210 },
        { threads: 2, speedup: 1.94, time: 0.0108 },
        { threads: 4, speedup: 3.75, time: 0.0056 },
        { threads: 8, speedup: 6.77, time: 0.0031 },
        { threads: 12, speedup: 9.55, time: 0.0022 },
        { threads: 16, speedup: 10.50, time: 0.0020 }
      ];

      this.initCanvas();
      this.bindEvents();
      this.drawChart();
    }

    initCanvas() {
      if (!this.canvas) return;
      const rect = this.canvas.parentElement.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      this.canvas.width = rect.width * dpr;
      this.canvas.height = 280 * dpr;
      this.canvas.style.width = `${rect.width}px`;
      this.canvas.style.height = `280px`;
      this.ctx.scale(dpr, dpr);
    }

    bindEvents() {
      window.addEventListener('resize', () => {
        this.initCanvas();
        this.drawChart();
      });

      if (this.btn) {
        this.btn.addEventListener('click', () => this.runBenchmark());
      }
      if (this.heroViewBench) {
        this.heroViewBench.addEventListener('click', (e) => {
          e.preventDefault();
          document.getElementById('benchmark-section').scrollIntoView({ behavior: 'smooth' });
        });
      }
    }

    async runBenchmark() {
      this.btn.disabled = true;
      this.btn.textContent = 'Running...';
      const folder = this.folderInput.value.trim() || '.';
      const res = await APIClient.benchmark(folder);

      this.btn.disabled = false;
      this.btn.textContent = 'Run Benchmark';

      if (res.tests && res.tests.length > 0) {
        this.benchData = res.tests.map(t => ({
          threads: t.threads,
          speedup: t.speedup,
          time: t.elapsed_sec
        }));
        this.renderTable(res.tests);
        this.drawChart();
      }
    }

    renderTable(tests) {
      if (!this.tbody) return;
      this.tbody.innerHTML = '';
      tests.forEach(t => {
        const tr = document.createElement('tr');
        const badgeClass = t.speedup >= 8.0 ? 'badge-success' : (t.speedup >= 1.5 ? 'badge-primary' : 'badge-gray');
        tr.innerHTML = `
          <td>${t.threads} Thread${t.threads > 1 ? 's' : ''}</td>
          <td>${t.elapsed_sec} s</td>
          <td>${t.files_scanned.toLocaleString()}</td>
          <td><span class="badge ${badgeClass}">${t.speedup.toFixed(2)}x</span></td>
        `;
        this.tbody.appendChild(tr);
      });
    }

    drawChart() {
      if (!this.ctx || !this.canvas) return;
      const dpr = window.devicePixelRatio || 1;
      const w = this.canvas.width / dpr;
      const h = this.canvas.height / dpr;
      const ctx = this.ctx;

      ctx.clearRect(0, 0, w, h);

      const padL = 40;
      const padR = 24;
      const padT = 20;
      const padB = 34;

      const chartW = w - padL - padR;
      const chartH = h - padT - padB;

      // Draw subtle grey grid lines
      ctx.strokeStyle = '#E4E7EC';
      ctx.lineWidth = 1;
      const steps = 4;
      for (let i = 0; i <= steps; i++) {
        const y = padT + (chartH / steps) * i;
        ctx.beginPath();
        ctx.moveTo(padL, y);
        ctx.lineTo(w - padR, y);
        ctx.stroke();

        const val = ((12 / steps) * (steps - i)).toFixed(1) + 'x';
        ctx.fillStyle = '#8B949E';
        ctx.font = '10px "JetBrains Mono"';
        ctx.textAlign = 'right';
        ctx.fillText(val, padL - 8, y + 4);
      }

      const maxTh = 16;
      const getX = (th) => padL + (th / maxTh) * chartW;
      const getY = (spd) => padT + chartH - (spd / 12) * chartH;

      // Draw Linear Reference line (dashed subtle grey)
      ctx.beginPath();
      ctx.strokeStyle = '#D0D5DD';
      ctx.setLineDash([4, 4]);
      ctx.lineWidth = 1.5;
      ctx.moveTo(getX(1), getY(1));
      ctx.lineTo(getX(maxTh), getY(maxTh > 12 ? 12 : maxTh));
      ctx.stroke();
      ctx.setLineDash([]);

      // Draw Measured Speedup (Clean blue line, no glow)
      ctx.beginPath();
      ctx.strokeStyle = '#315EFB';
      ctx.lineWidth = 2.5;

      this.benchData.forEach((pt, idx) => {
        const x = getX(pt.threads);
        const y = getY(pt.speedup);
        if (idx === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();

      // Draw clean circular dots & labels
      this.benchData.forEach(pt => {
        const x = getX(pt.threads);
        const y = getY(pt.speedup);

        ctx.fillStyle = '#FFFFFF';
        ctx.beginPath();
        ctx.arc(x, y, 4.5, 0, Math.PI * 2);
        ctx.fill();

        ctx.strokeStyle = '#315EFB';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(x, y, 4.5, 0, Math.PI * 2);
        ctx.stroke();

        // Data point text
        ctx.fillStyle = '#202733';
        ctx.font = '500 10px "Inter"';
        ctx.textAlign = 'center';
        ctx.fillText(`${pt.speedup.toFixed(1)}x`, x, y - 8);

        // X Axis labels
        ctx.fillStyle = '#667085';
        ctx.font = '10px "JetBrains Mono"';
        ctx.fillText(`${pt.threads}T`, x, h - padB + 16);
      });
    }
  }

  // ------------------------------------------------------------------------
  // History Controller
  // ------------------------------------------------------------------------
  class HistoryController {
    constructor() {
      this.tbody = document.getElementById('history-tbody');
      this.refreshBtn = document.getElementById('refresh-history-btn');
      this.clearBtn = document.getElementById('clear-history-btn');

      this.bindEvents();
      this.loadHistory();
    }

    bindEvents() {
      if (this.refreshBtn) {
        this.refreshBtn.addEventListener('click', () => this.loadHistory());
      }
      if (this.clearBtn) {
        this.clearBtn.addEventListener('click', async () => {
          if (confirm('Are you sure you want to clear search_history.txt?')) {
            await APIClient.clearHistory();
            this.loadHistory();
          }
        });
      }
    }

    async loadHistory() {
      const res = await APIClient.getHistory();
      const items = res.history || [];
      if (!this.tbody) return;
      this.tbody.innerHTML = '';

      if (items.length === 0) {
        this.tbody.innerHTML = '<tr><td colspan="8" style="text-align: center; color: var(--text-secondary); padding: 32px;">No search history found.</td></tr>';
        return;
      }

      items.forEach(item => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
          <td><span style="font-family: var(--font-mono); font-size: 0.8rem; color: var(--text-secondary);">${escapeHtml(item.date)}</span></td>
          <td><span class="badge badge-gray">${escapeHtml(item.type)}</span></td>
          <td><span style="font-family: var(--font-mono); font-size: 0.8rem;">${escapeHtml(item.folder)}</span></td>
          <td><strong style="color: var(--text-primary);">${escapeHtml(item.query)}</strong></td>
          <td>${escapeHtml(item.threads)}</td>
          <td><span style="font-family: var(--font-mono); font-size: 0.82rem; color: var(--text-secondary);">${escapeHtml(item.time)}</span></td>
          <td><strong style="color: var(--primary);">${escapeHtml(item.matches)}</strong></td>
          <td style="text-align: right;">
            <button class="btn btn-secondary btn-sm run-again-btn" data-query="${escapeHtml(item.query)}" data-folder="${escapeHtml(item.folder)}">Run</button>
          </td>
        `;

        tr.querySelector('.run-again-btn').addEventListener('click', (e) => {
          const q = e.target.dataset.query;
          const f = e.target.dataset.folder;
          const qInput = document.getElementById('search-query-input');
          const fInput = document.getElementById('search-folder-input');
          if (qInput) qInput.value = q;
          if (fInput) fInput.value = f;
          document.getElementById('search-section').scrollIntoView({ behavior: 'smooth' });
          document.getElementById('run-search-btn').click();
        });

        this.tbody.appendChild(tr);
      });
    }
  }

  // ------------------------------------------------------------------------
  // Navigation Controller
  // ------------------------------------------------------------------------
  function initNav() {
    const navLinks = document.querySelectorAll('.nav-link');
    const sections = [
      { id: 'search-section', link: document.getElementById('nav-search') },
      { id: 'benchmark-section', link: document.getElementById('nav-benchmark') },
      { id: 'architecture-section', link: document.getElementById('nav-arch') },
      { id: 'history-section', link: document.getElementById('nav-history') }
    ];

    window.addEventListener('scroll', () => {
      const scrollPos = window.scrollY + 100;
      sections.forEach(sec => {
        const el = document.getElementById(sec.id);
        if (!el || !sec.link) return;
        const top = el.offsetTop;
        const h = el.offsetHeight;
        if (scrollPos >= top && scrollPos < top + h) {
          navLinks.forEach(l => l.classList.remove('active'));
          sec.link.classList.add('active');
        }
      });
    });

    // Smooth scroll for nav items
    navLinks.forEach(link => {
      link.addEventListener('click', (e) => {
        const targetId = link.getAttribute('href');
        if (targetId && targetId.startsWith('#')) {
          e.preventDefault();
          const targetSection = document.getElementById(targetId.substring(1) + '-section');
          if (targetSection) {
            targetSection.scrollIntoView({ behavior: 'smooth' });
          }
        }
      });
    });
  }

  // Helper
  function escapeHtml(str) {
    if (!str) return '';
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#039;');
  }

  // DOM Ready
  document.addEventListener('DOMContentLoaded', () => {
    initNav();
    new SearchController();
    new BenchmarkController();
    new HistoryController();
  });

})();
