using System;
using System.Drawing;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Windows.Forms;

namespace MapEditor;

public sealed class MainForm : Form
{
    // ── Canvas ────────────────────────────────────────────────────────────────
    private readonly MapCanvas _canvas;

    // ── State ─────────────────────────────────────────────────────────────────
    private string? _filePath;
    private bool    _isDirty;
    private bool    _syncingUI;   // prevents re-entrant UI updates

    // ── Map property controls ─────────────────────────────────────────────────
    private TextBox _txtName    = null!;
    private TextBox _txtBoundsX = null!, _txtBoundsY = null!;
    private TextBox _txtBoundsW = null!, _txtBoundsH = null!;
    private TextBox _txtSpawnX  = null!, _txtSpawnY  = null!;

    // ── Platform property controls ────────────────────────────────────────────
    private Panel   _pnlPlatform = null!;
    private TextBox _txtPlatX = null!, _txtPlatY = null!;
    private TextBox _txtPlatW = null!, _txtPlatH = null!;
    private TextBox _txtPlatR = null!, _txtPlatG = null!, _txtPlatB = null!;
    private Panel   _colorSwatch = null!;

    // ── Toolbar buttons ───────────────────────────────────────────────────────
    private ToolStripButton _btnSelect = null!, _btnDraw = null!, _btnSnap = null!;

    // ── Status bar labels ─────────────────────────────────────────────────────
    private ToolStripStatusLabel _lblInfo  = null!;
    private ToolStripStatusLabel _lblZoom  = null!;
    private ToolStripStatusLabel _lblCoord = null!;

    // ─────────────────────────────────────────────────────────────────────────
    public MainForm()
    {
        Text        = "Map Editor";
        Size        = new Size(1200, 800);
        MinimumSize = new Size(900, 600);
        BackColor   = Color.FromArgb(45, 45, 50);
        _canvas     = new MapCanvas();
        BuildUI();
        NewMap();
    }

    // ── UI construction ───────────────────────────────────────────────────────
    private void BuildUI()
    {
        SuspendLayout();

        MainMenuStrip = BuildMenu();
        var toolbar = BuildToolbar();
        // Add toolbar before menu strip so the menu bar docks above the toolbar
        // (WinForms DockStyle.Top: later-added controls dock higher).
        Controls.Add(toolbar);
        Controls.Add(MainMenuStrip);
        Controls.Add(BuildStatusBar());

        // Properties panel (left) + splitter must be added before the canvas
        // so DockStyle.Left stacks correctly (last-added DockStyle.Fill wins).
        var left = BuildPropertiesPanel();
        Controls.Add(left);
        Controls.Add(new Splitter { Dock = DockStyle.Left, Width = 4, BackColor = Color.FromArgb(60, 60, 65) });

        _canvas.Dock             = DockStyle.Fill;
        _canvas.SelectionChanged += OnSelectionChanged;
        _canvas.MapChanged       += OnMapChanged;
        _canvas.ZoomChanged      += OnZoomChanged;
        _canvas.MouseMove        += (_, me) =>
        {
            var w = _canvas.WorldAt(me.Location);
            _lblCoord.Text = $"({w.X:F0}, {w.Y:F0})";
        };
        Controls.Add(_canvas);

        KeyPreview = true;
        KeyDown   += OnFormKeyDown;
        ResumeLayout();
    }

    private MenuStrip BuildMenu()
    {
        var menu = new MenuStrip();
        var file = new ToolStripMenuItem("&File");
        var edit = new ToolStripMenuItem("&Edit");
        var view = new ToolStripMenuItem("&View");

        file.DropDownItems.Add("&New\tCtrl+N",     null, (_, _) => NewMap());
        file.DropDownItems.Add("&Open…\tCtrl+O",   null, (_, _) => OpenFile());
        file.DropDownItems.Add("&Save\tCtrl+S",    null, (_, _) => SaveFile());
        file.DropDownItems.Add("Save &As…",        null, (_, _) => SaveFileAs());
        file.DropDownItems.Add(new ToolStripSeparator());
        file.DropDownItems.Add("E&xit",             null, (_, _) => Close());

        edit.DropDownItems.Add("&Delete Platform\tDel", null, (_, _) => _canvas.DeleteSelected());

        view.DropDownItems.Add("&Fit to View\tF",       null, (_, _) => _canvas.FitToView());
        view.DropDownItems.Add("Zoom 100%\t1",          null, (_, _) => _canvas.SetZoom(1f));
        view.DropDownItems.Add("Zoom In\t=",            null, (_, _) => _canvas.SetZoom(_canvas.Zoom * 1.25f));
        view.DropDownItems.Add("Zoom Out\t-",           null, (_, _) => _canvas.SetZoom(_canvas.Zoom / 1.25f));
        view.DropDownItems.Add(new ToolStripSeparator());
        view.DropDownItems.Add("Toggle Grid &Snap\tG",  null, (_, _) => ToggleSnap());

        menu.Items.AddRange(new ToolStripItem[] { file, edit, view });
        return menu;
    }

    private ToolStrip BuildToolbar()
    {
        var bar = new ToolStrip { GripStyle = ToolStripGripStyle.Hidden };

        _btnSelect = Tbtn("Select",   "Select / Move / Resize  [S]", true);
        _btnDraw   = Tbtn("Draw",     "Draw new platform  [D]",      false);
        _btnSnap   = Tbtn("Snap: ON", "Toggle grid snapping  [G]",   true);

        _btnSelect.Click += (_, _) => SetTool(EditorTool.Select);
        _btnDraw.Click   += (_, _) => SetTool(EditorTool.Draw);
        _btnSnap.Click   += (_, _) => ToggleSnap();

        var btnDel = new ToolStripButton("Delete") { ToolTipText = "Delete selected  [Del]" };
        btnDel.Click += (_, _) => _canvas.DeleteSelected();

        var btnFit = new ToolStripButton("Fit View") { ToolTipText = "Fit map in view  [F]" };
        btnFit.Click += (_, _) => _canvas.FitToView();

        var btn100 = new ToolStripButton("100%") { ToolTipText = "Zoom to 100%  [1]" };
        btn100.Click += (_, _) => _canvas.SetZoom(1f);

        bar.Items.AddRange(new ToolStripItem[]
        {
            _btnSelect, _btnDraw,
            new ToolStripSeparator(),
            btnDel,
            new ToolStripSeparator(),
            btnFit, btn100,
            new ToolStripSeparator(),
            _btnSnap
        });
        return bar;

        static ToolStripButton Tbtn(string text, string tip, bool chk) =>
            new(text) { ToolTipText = tip, Checked = chk, CheckOnClick = false };
    }

    private StatusStrip BuildStatusBar()
    {
        var bar = new StatusStrip();
        _lblInfo  = new ToolStripStatusLabel("Ready") { Spring = true, TextAlign = ContentAlignment.MiddleLeft };
        _lblZoom  = new ToolStripStatusLabel("Zoom: 100%") { AutoSize = false, Width = 90 };
        _lblCoord = new ToolStripStatusLabel("(0, 0)")     { AutoSize = false, Width = 110 };
        bar.Items.AddRange(new ToolStripItem[] { _lblInfo, _lblZoom, _lblCoord });
        return bar;
    }

    // ── Properties panel ──────────────────────────────────────────────────────
    private Panel BuildPropertiesPanel()
    {
        var panel = new Panel
        {
            Width      = 230,
            Dock       = DockStyle.Left,
            BackColor  = Color.FromArgb(48, 48, 53),
            AutoScroll = true
        };

        int y = 8;
        const int pw = 214;   // usable pixel width inside padding

        // ── Map Settings ──────────────────────────────────────────────────────
        panel.Controls.Add(SectionLabel("MAP SETTINGS", 4, y, pw)); y += 22;

        (_, _txtName,    y) = SingleField(panel, "Name",     y, pw);
        (_txtBoundsX, _txtBoundsY, y) = TwoFields(panel, "X", "Y", y, pw, "Bounds origin");
        (_txtBoundsW, _txtBoundsH, y) = TwoFields(panel, "Width", "Height", y, pw, "Bounds size");
        (_txtSpawnX,  _txtSpawnY,  y) = TwoFields(panel, "X", "Y", y, pw, "Spawn point");

        var btnMap = DarkBtn("Apply Map Settings", 4, y, pw); y += 30;
        btnMap.Click += ApplyMapSettings;
        panel.Controls.Add(btnMap);

        y += 8;
        panel.Controls.Add(new Label { Height = 1, Location = new(4, y), Width = pw, BackColor = Color.FromArgb(70, 70, 80) });
        y += 8;

        // ── Selected Platform ─────────────────────────────────────────────────
        panel.Controls.Add(SectionLabel("SELECTED PLATFORM", 4, y, pw)); y += 22;

        _pnlPlatform = new Panel { Location = new(0, y), Width = panel.Width, AutoSize = false };
        panel.Controls.Add(_pnlPlatform);

        int py = 0;
        (_txtPlatX, _txtPlatY, py) = TwoFields(_pnlPlatform, "X", "Y",           py, pw, "Position");
        (_txtPlatW, _txtPlatH, py) = TwoFields(_pnlPlatform, "Width", "Height",   py, pw, "Size");
        (_txtPlatR, _txtPlatG, _txtPlatB, py) = ThreeFields(_pnlPlatform, "R", "G", "B", py, pw, "Color (R G B, 0–255)");

        _colorSwatch = new Panel
        {
            Location    = new(4, py),
            Size        = new(pw, 20),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor   = Color.FromArgb(100, 100, 100)
        };
        _pnlPlatform.Controls.Add(_colorSwatch);
        py += 26;

        int half = (pw - 4) / 2;
        var btnApply = DarkBtn("Apply",  4,           py, half);
        var btnDel   = DarkBtn("Delete", 4 + half + 4, py, half, Color.FromArgb(130, 55, 55));
        py += 30;

        btnApply.Click += ApplyPlatformSettings;
        btnDel.Click   += (_, _) => _canvas.DeleteSelected();
        _pnlPlatform.Controls.AddRange(new Control[] { btnApply, btnDel });
        _pnlPlatform.Height = py;

        // Live colour swatch
        _txtPlatR.TextChanged += (_, _) => RefreshSwatch();
        _txtPlatG.TextChanged += (_, _) => RefreshSwatch();
        _txtPlatB.TextChanged += (_, _) => RefreshSwatch();

        _pnlPlatform.Enabled = false;
        return panel;
    }

    // ── Control factory helpers ───────────────────────────────────────────────
    private static Label SectionLabel(string text, int x, int y, int w) => new()
    {
        Text      = text,
        Font      = new Font("Segoe UI", 8f, FontStyle.Bold),
        ForeColor = Color.FromArgb(155, 155, 175),
        Location  = new(x, y),
        Size      = new(w, 18),
        AutoSize  = false
    };

    private static (Label lbl, TextBox txt, int nextY) SingleField(
        Control parent, string label, int y, int w)
    {
        var lbl = new Label
        {
            Text = label + ":", ForeColor = Color.Silver,
            Font = new Font("Segoe UI", 7.5f),
            Location = new(4, y), Size = new(w, 15), AutoSize = false
        };
        var txt = DarkTxt(4, y + 15, w);
        parent.Controls.AddRange(new Control[] { lbl, txt });
        return (lbl, txt, y + 38);
    }

    private static (TextBox a, TextBox b, int nextY) TwoFields(
        Control parent, string lA, string lB, int y, int w, string? header = null)
    {
        int half = (w - 4) / 2;
        if (header != null)
        {
            parent.Controls.Add(new Label
            {
                Text = header + ":", ForeColor = Color.Silver,
                Font = new Font("Segoe UI", 7.5f),
                Location = new(4, y), Size = new(w, 15), AutoSize = false
            });
            y += 15;
        }
        parent.Controls.Add(SubLabel(lA, 4, y, half));
        parent.Controls.Add(SubLabel(lB, 4 + half + 4, y, half));
        var ta = DarkTxt(4,             y + 13, half);
        var tb = DarkTxt(4 + half + 4,  y + 13, half);
        parent.Controls.AddRange(new Control[] { ta, tb });
        return (ta, tb, y + 36);
    }

    private static (TextBox a, TextBox b, TextBox c, int nextY) ThreeFields(
        Control parent, string lA, string lB, string lC, int y, int w, string? header = null)
    {
        int third = (w - 8) / 3;
        if (header != null)
        {
            parent.Controls.Add(new Label
            {
                Text = header + ":", ForeColor = Color.Silver,
                Font = new Font("Segoe UI", 7.5f),
                Location = new(4, y), Size = new(w, 15), AutoSize = false
            });
            y += 15;
        }
        parent.Controls.Add(SubLabel(lA, 4,                  y, third));
        parent.Controls.Add(SubLabel(lB, 4 + third + 4,      y, third));
        parent.Controls.Add(SubLabel(lC, 4 + (third + 4) * 2, y, third));
        var ta = DarkTxt(4,                   y + 13, third);
        var tb = DarkTxt(4 + third + 4,       y + 13, third);
        var tc = DarkTxt(4 + (third + 4) * 2, y + 13, third);
        parent.Controls.AddRange(new Control[] { ta, tb, tc });
        return (ta, tb, tc, y + 36);
    }

    private static Label SubLabel(string text, int x, int y, int w) => new()
    {
        Text = text, ForeColor = Color.DarkGray,
        Font = new Font("Segoe UI", 7f),
        Location = new(x, y), Size = new(w, 13), AutoSize = false
    };

    private static TextBox DarkTxt(int x, int y, int w) => new()
    {
        Location    = new(x, y),
        Size        = new(w, 22),
        BackColor   = Color.FromArgb(58, 58, 65),
        ForeColor   = Color.WhiteSmoke,
        BorderStyle = BorderStyle.FixedSingle,
        Font        = new Font("Consolas", 8.5f)
    };

    private static Button DarkBtn(string text, int x, int y, int w, Color? back = null) => new()
    {
        Text      = text,
        Location  = new(x, y),
        Size      = new(w, 24),
        BackColor = back ?? Color.FromArgb(65, 95, 125),
        ForeColor = Color.White,
        FlatStyle = FlatStyle.Flat,
        Font      = new Font("Segoe UI", 8f)
    };

    // ── File commands ─────────────────────────────────────────────────────────
    private void NewMap()
    {
        if (!ConfirmDiscard()) return;
        var map = new MapData
        {
            Name       = "New Map",
            Bounds     = new() { X = -200, Y = -500, Width = 3600, Height = 1200 },
            SpawnPoint = new() { X = 150, Y = 475 },
            Platforms  = new()
            {
                new() { X = -200, Y = 500, Width = 3600, Height = 40, R = 80, G = 80, B = 80 }
            }
        };
        _filePath = null;
        _isDirty  = false;
        _canvas.LoadMap(map);
        SyncMapFields(map);
        UpdateTitle();
        SetStatus("New map created.");
    }

    private void OpenFile()
    {
        if (!ConfirmDiscard()) return;
        using var dlg = new OpenFileDialog
        {
            Title            = "Open Map",
            Filter           = "JSON map files (*.json)|*.json|All files (*.*)|*.*",
            InitialDirectory = MapsDir()
        };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        try
        {
            var json = File.ReadAllText(dlg.FileName);
            var opts = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
            var map  = JsonSerializer.Deserialize<MapData>(json, opts)
                       ?? throw new InvalidDataException("Failed to parse map file.");
            _filePath = dlg.FileName;
            _isDirty  = false;
            _canvas.LoadMap(map);
            SyncMapFields(map);
            UpdateTitle();
            SetStatus($"Opened: {Path.GetFileName(_filePath)}");
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, $"Could not open file:\n{ex.Message}",
                "Open Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void SaveFile()
    {
        if (_filePath == null) { SaveFileAs(); return; }
        WriteFile(_filePath);
    }

    private void SaveFileAs()
    {
        using var dlg = new SaveFileDialog
        {
            Title            = "Save Map",
            Filter           = "JSON map files (*.json)|*.json|All files (*.*)|*.*",
            FileName         = _filePath != null ? Path.GetFileName(_filePath) : "world_01.json",
            InitialDirectory = MapsDir()
        };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        _filePath = dlg.FileName;
        WriteFile(_filePath);
    }

    private void WriteFile(string path)
    {
        if (_canvas.Map == null) return;
        try
        {
            var map = _canvas.Map;

            // Temporarily null-out empty collections so they are omitted from JSON
            var savedSpawnPts = map.SpawnPoints;
            var savedEnemies  = map.Enemies;
            var savedTrans    = map.Transitions;
            var savedPickups  = map.AbilityPickups;
            if (savedSpawnPts.Count == 0) map.SpawnPoints    = null!;
            if (savedEnemies.Count  == 0) map.Enemies        = null!;
            if (savedTrans.Count    == 0) map.Transitions    = null!;
            if (savedPickups.Count  == 0) map.AbilityPickups = null!;

            var opts = new JsonSerializerOptions
            {
                WriteIndented = true,
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
            };
            var json = JsonSerializer.Serialize(map, opts);
            File.WriteAllText(path, json);

            // Restore collections so in-memory model stays usable
            map.SpawnPoints    = savedSpawnPts;
            map.Enemies        = savedEnemies;
            map.Transitions    = savedTrans;
            map.AbilityPickups = savedPickups;

            _isDirty = false;
            UpdateTitle();
            SetStatus($"Saved: {Path.GetFileName(path)}");
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, $"Could not save file:\n{ex.Message}",
                "Save Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private static string MapsDir()
    {
        // Walk up from the executable to find the maps/ folder automatically.
        string dir = AppDomain.CurrentDomain.BaseDirectory;
        for (int i = 0; i < 6; i++)
        {
            string candidate = Path.Combine(dir, "maps");
            if (Directory.Exists(candidate)) return candidate;
            string? parent = Path.GetDirectoryName(dir);
            if (parent == null) break;
            dir = parent;
        }
        return AppDomain.CurrentDomain.BaseDirectory;
    }

    // ── Apply settings ────────────────────────────────────────────────────────
    private void ApplyMapSettings(object? sender, EventArgs e)
    {
        var map = _canvas.Map;
        if (map == null) return;
        try
        {
            map.Name          = _txtName.Text.Trim();
            map.Bounds.X      = ParseF(_txtBoundsX);
            map.Bounds.Y      = ParseF(_txtBoundsY);
            map.Bounds.Width  = ParseF(_txtBoundsW);
            map.Bounds.Height = ParseF(_txtBoundsH);
            map.SpawnPoint.X  = ParseF(_txtSpawnX);
            map.SpawnPoint.Y  = ParseF(_txtSpawnY);
            MarkDirty();
            _canvas.Invalidate();
            SetStatus("Map settings applied.");
        }
        catch { Warn("Invalid values in map settings. Use numeric values."); }
    }

    private void ApplyPlatformSettings(object? sender, EventArgs e)
    {
        var p = _canvas.SelectedPlatform;
        if (p == null) return;
        try
        {
            p.X      = ParseF(_txtPlatX);
            p.Y      = ParseF(_txtPlatY);
            p.Width  = MathF.Max(1f, ParseF(_txtPlatW));
            p.Height = MathF.Max(1f, ParseF(_txtPlatH));
            p.R      = Math.Clamp(ParseI(_txtPlatR), 0, 255);
            p.G      = Math.Clamp(ParseI(_txtPlatG), 0, 255);
            p.B      = Math.Clamp(ParseI(_txtPlatB), 0, 255);
            MarkDirty();
            _canvas.Invalidate();
            RefreshSwatch();
            SetStatus("Platform updated.");
        }
        catch { Warn("Invalid values in platform settings. Use numeric values."); }
    }

    // ── Canvas event handlers ─────────────────────────────────────────────────
    private void OnSelectionChanged(object? sender, EventArgs e)
    {
        var p = _canvas.SelectedPlatform;
        _pnlPlatform.Enabled = p != null;
        if (p != null)
        {
            _syncingUI = true;
            _txtPlatX.Text = p.X.ToString("F1");
            _txtPlatY.Text = p.Y.ToString("F1");
            _txtPlatW.Text = p.Width.ToString("F1");
            _txtPlatH.Text = p.Height.ToString("F1");
            _txtPlatR.Text = p.R.ToString();
            _txtPlatG.Text = p.G.ToString();
            _txtPlatB.Text = p.B.ToString();
            _syncingUI = false;
            RefreshSwatch();
        }
        UpdateStatus();
    }

    private void OnMapChanged(object? sender, EventArgs e)
    {
        // Keep the position/size fields live while dragging
        var p = _canvas.SelectedPlatform;
        if (p != null && !_syncingUI)
        {
            _syncingUI = true;
            _txtPlatX.Text = p.X.ToString("F1");
            _txtPlatY.Text = p.Y.ToString("F1");
            _txtPlatW.Text = p.Width.ToString("F1");
            _txtPlatH.Text = p.Height.ToString("F1");
            _syncingUI = false;
        }
        MarkDirty();
    }

    private void OnZoomChanged(object? sender, EventArgs e)
    {
        _lblZoom.Text = $"Zoom: {_canvas.Zoom * 100:F0}%";
    }

    // ── Tool helpers ──────────────────────────────────────────────────────────
    private void SetTool(EditorTool t)
    {
        _canvas.Tool       = t;
        _btnSelect.Checked = t == EditorTool.Select;
        _btnDraw.Checked   = t == EditorTool.Draw;
        UpdateStatus();
    }

    private void ToggleSnap()
    {
        _canvas.SnapToGrid = !_canvas.SnapToGrid;
        _btnSnap.Checked   = _canvas.SnapToGrid;
        _btnSnap.Text      = _canvas.SnapToGrid ? "Snap: ON" : "Snap: OFF";
        SetStatus($"Grid snap {(_canvas.SnapToGrid ? "enabled" : "disabled")}.");
    }

    // ── Status / title ────────────────────────────────────────────────────────
    private void UpdateStatus()
    {
        int    count = _canvas.Map?.Platforms.Count ?? 0;
        string tool  = _canvas.Tool == EditorTool.Draw ? "Draw" : "Select";
        string sel   = _canvas.SelectedPlatform != null ? " | 1 selected" : "";
        _lblInfo.Text = $"{count} platform{(count != 1 ? "s" : "")}{sel} | {tool} mode";
    }

    private void SetStatus(string msg) => _lblInfo.Text = msg;

    private void MarkDirty()
    {
        if (_isDirty) return;
        _isDirty = true;
        UpdateTitle();
        UpdateStatus();
    }

    private void UpdateTitle()
    {
        string name = _filePath != null ? Path.GetFileName(_filePath) : "Untitled";
        Text = $"Map Editor — {name}{(_isDirty ? " *" : "")}";
    }

    // ── UI sync ───────────────────────────────────────────────────────────────
    private void SyncMapFields(MapData map)
    {
        _syncingUI      = true;
        _txtName.Text   = map.Name;
        _txtBoundsX.Text = map.Bounds.X.ToString("F1");
        _txtBoundsY.Text = map.Bounds.Y.ToString("F1");
        _txtBoundsW.Text = map.Bounds.Width.ToString("F1");
        _txtBoundsH.Text = map.Bounds.Height.ToString("F1");
        _txtSpawnX.Text  = map.SpawnPoint.X.ToString("F1");
        _txtSpawnY.Text  = map.SpawnPoint.Y.ToString("F1");
        _syncingUI = false;
        UpdateStatus();
    }

    private void RefreshSwatch()
    {
        if (!int.TryParse(_txtPlatR.Text, out int r)) return;
        if (!int.TryParse(_txtPlatG.Text, out int g)) return;
        if (!int.TryParse(_txtPlatB.Text, out int b)) return;
        _colorSwatch.BackColor = Color.FromArgb(
            Math.Clamp(r, 0, 255), Math.Clamp(g, 0, 255), Math.Clamp(b, 0, 255));
    }

    // ── Keyboard shortcuts ────────────────────────────────────────────────────
    private void OnFormKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Control)
        {
            switch (e.KeyCode)
            {
                case Keys.N: NewMap();    e.Handled = true; return;
                case Keys.O: OpenFile();  e.Handled = true; return;
                case Keys.S: SaveFile();  e.Handled = true; return;
            }
        }
        switch (e.KeyCode)
        {
            case Keys.S: SetTool(EditorTool.Select); e.Handled = true; break;
            case Keys.D: SetTool(EditorTool.Draw);   e.Handled = true; break;
            case Keys.F: _canvas.FitToView();         e.Handled = true; break;
            case Keys.G: ToggleSnap();                e.Handled = true; break;
            case Keys.D1: _canvas.SetZoom(1f);        e.Handled = true; break;
            case Keys.OemMinus: _canvas.SetZoom(_canvas.Zoom / 1.25f); e.Handled = true; break;
            case Keys.Oemplus:  _canvas.SetZoom(_canvas.Zoom * 1.25f); e.Handled = true; break;
        }
    }

    // ── Close guard ───────────────────────────────────────────────────────────
    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        if (_isDirty && !ConfirmDiscard()) e.Cancel = true;
        base.OnFormClosing(e);
    }

    // ── Utilities ─────────────────────────────────────────────────────────────
    private bool ConfirmDiscard()
    {
        if (!_isDirty) return true;
        return MessageBox.Show(this,
            "You have unsaved changes. Discard them?", "Unsaved Changes",
            MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes;
    }

    private static float ParseF(TextBox t) => float.Parse(t.Text.Trim());
    private static int   ParseI(TextBox t) => int.Parse(t.Text.Trim());
    private void Warn(string msg) =>
        MessageBox.Show(this, msg, "Invalid Input", MessageBoxButtons.OK, MessageBoxIcon.Warning);
}
