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

    // ── World state ──────────────────────────────────────────────────────────
    private WorldData? _worldData;
    private string?    _worldFilePath;

    // ── Map property controls ─────────────────────────────────────────────────
    private TextBox _txtName    = null!;
    private TextBox _txtBoundsX = null!, _txtBoundsY = null!;
    private TextBox _txtBoundsW = null!, _txtBoundsH = null!;

    // ── Spawn point controls ─────────────────────────────────────────────────
    private Panel   _pnlSpawn = null!;
    private ListBox _lstSpawnPoints = null!;
    private TextBox _txtSpawnName   = null!, _txtSpawnPtX = null!, _txtSpawnPtY = null!;
    private Label   _lblSpawnTitle  = null!;
    private Button  _btnSpawnDel    = null!;

    // ── Platform property controls ────────────────────────────────────────────
    private Panel   _pnlPlatform = null!;
    private TextBox _txtPlatX = null!, _txtPlatY = null!;
    private TextBox _txtPlatW = null!, _txtPlatH = null!;
    private TextBox _txtPlatR = null!, _txtPlatG = null!, _txtPlatB = null!;
    private Panel   _colorSwatch = null!;

    // ── Enemy property controls ───────────────────────────────────────────────
    private Panel   _pnlEnemy = null!;
    private TextBox _txtEnemyX = null!, _txtEnemyY = null!;
    private TextBox _txtEnemyW = null!, _txtEnemyH = null!;
    private TextBox _txtWaypointAX = null!, _txtWaypointAY = null!;
    private TextBox _txtWaypointBX = null!, _txtWaypointBY = null!;
    private TextBox _txtEnemySpeed = null!, _txtEnemyDamage = null!, _txtEnemyHP = null!;

    // ── Transition property controls ──────────────────────────────────────────
    // (Removed: transitions are now auto-generated and not editable)

    // ── Pickup property controls ──────────────────────────────────────────────
    private Panel    _pnlPickup = null!;
    private TextBox  _txtPickupId = null!, _txtPickupX = null!, _txtPickupY = null!;
    private TextBox  _txtPickupW = null!, _txtPickupH = null!;
    private ComboBox _cboPickupAbility = null!;

    // ── Maps panel controls ──────────────────────────────────────────────────
    private ListBox _lstMaps    = null!;
    private Button  _btnAddMap  = null!;
    private Button  _btnNewMap  = null!;
    private Button  _btnRemMap  = null!;
    private bool    _worldStructureDirty;  // maps added/removed/repositioned

    // ── Toolbar buttons ───────────────────────────────────────────────────────
    private ToolStripButton _btnSelect = null!, _btnDraw = null!, _btnDrawEnemy = null!, _btnDrawPickup = null!, _btnDrawSpawn = null!, _btnMoveMap = null!, _btnSnap = null!;

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
        _canvas.ActiveMapChanged += OnActiveMapChanged;
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
        file.DropDownItems.Add("New &World\tCtrl+Shift+N",      null, (_, _) => NewWorld());
        file.DropDownItems.Add("Open W&orld…\tCtrl+Shift+O",    null, (_, _) => OpenWorld());
        file.DropDownItems.Add("Save Wo&rld\tCtrl+Shift+S",     null, (_, _) => SaveWorld());
        file.DropDownItems.Add("Save World As…",                null, (_, _) => SaveWorldAs());
        file.DropDownItems.Add(new ToolStripSeparator());
        file.DropDownItems.Add("&Add Map to World…",     null, (_, _) => AddMapToWorld());
        file.DropDownItems.Add("New Map in Worl&d…",     null, (_, _) => NewMapInWorld());
        file.DropDownItems.Add("&Remove Map from World", null, (_, _) => RemoveMapFromWorld());
        file.DropDownItems.Add(new ToolStripSeparator());
        file.DropDownItems.Add("E&xit",             null, (_, _) => Close());

        edit.DropDownItems.Add("&Delete Selected\tDel", null, (_, _) => _canvas.DeleteSelected());

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

        _btnSelect         = Tbtn("Select",          "Select / Move / Resize  [S]", true);
        _btnDraw           = Tbtn("Draw",            "Draw new platform  [D]",      false);
        _btnDrawEnemy      = Tbtn("Draw Enemy",      "Draw new enemy  [E]",         false);
        _btnDrawPickup     = Tbtn("Draw Pickup",     "Draw new ability pickup  [P]", false);
        _btnDrawSpawn      = Tbtn("Draw Spawn",      "Draw new spawn point  [W]",   false);
        _btnMoveMap        = Tbtn("Move Map",        "Move map in world space  [M]", false);
        _btnSnap           = Tbtn("Snap: ON",        "Toggle grid snapping  [G]",   true);

        _btnSelect.Click         += (_, _) => SetTool(EditorTool.Select);
        _btnDraw.Click           += (_, _) => SetTool(EditorTool.Draw);
        _btnDrawEnemy.Click      += (_, _) => SetTool(EditorTool.DrawEnemy);
        _btnDrawPickup.Click     += (_, _) => SetTool(EditorTool.DrawPickup);
        _btnDrawSpawn.Click      += (_, _) => SetTool(EditorTool.DrawSpawnPoint);
        _btnMoveMap.Click        += (_, _) => SetTool(EditorTool.MoveMap);
        _btnSnap.Click           += (_, _) => ToggleSnap();

        var btnDel = new ToolStripButton("Delete") { ToolTipText = "Delete selected  [Del]" };
        btnDel.Click += (_, _) => _canvas.DeleteSelected();

        var btnFit = new ToolStripButton("Fit View") { ToolTipText = "Fit map in view  [F]" };
        btnFit.Click += (_, _) => _canvas.FitToView();

        var btn100 = new ToolStripButton("100%") { ToolTipText = "Zoom to 100%  [1]" };
        btn100.Click += (_, _) => _canvas.SetZoom(1f);

        bar.Items.AddRange(new ToolStripItem[]
        {
            _btnSelect, _btnDraw, _btnDrawEnemy, _btnDrawPickup, _btnDrawSpawn, _btnMoveMap,
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

        // ── Map Settings (always visible) ─────────────────────────────────────
        panel.Controls.Add(SectionLabel("MAP SETTINGS", 4, y, pw)); y += 22;

        (_, _txtName,    y) = SingleField(panel, "Name",     y, pw);
        (_txtBoundsX, _txtBoundsY, y) = TwoFields(panel, "X", "Y", y, pw, "Bounds origin");
        (_txtBoundsW, _txtBoundsH, y) = TwoFields(panel, "Width", "Height", y, pw, "Bounds size");

        var btnMap = DarkBtn("Apply Map Settings", 4, y, pw); y += 30;
        btnMap.Click += ApplyMapSettings;
        panel.Controls.Add(btnMap);

        y += 8;
        panel.Controls.Add(new Label { Height = 1, Location = new(4, y), Width = pw, BackColor = Color.FromArgb(70, 70, 80) });
        y += 8;

        // ── Dynamic panels (only one visible at a time) ──────────────────────
        int dynamicY = y;

        // ── Spawn Point panel ─────────────────────────────────────────────────
        _pnlSpawn = new Panel { Location = new(0, dynamicY), Width = panel.Width, AutoSize = false, Visible = false };
        panel.Controls.Add(_pnlSpawn);

        int sy = 0;
        _lblSpawnTitle = SectionLabel("SPAWN POINT", 4, sy, pw);
        _pnlSpawn.Controls.Add(_lblSpawnTitle); sy += 22;

        _lstSpawnPoints = new ListBox
        {
            Location    = new(4, sy),
            Size        = new(pw, 80),
            BackColor   = Color.FromArgb(58, 58, 65),
            ForeColor   = Color.WhiteSmoke,
            BorderStyle = BorderStyle.FixedSingle,
            Font        = new Font("Consolas", 8.5f)
        };
        _lstSpawnPoints.SelectedIndexChanged += OnSpawnListSelected;
        _pnlSpawn.Controls.Add(_lstSpawnPoints);
        sy += 84;

        (_, _txtSpawnName, sy) = SingleField(_pnlSpawn, "Name", sy, pw);
        (_txtSpawnPtX, _txtSpawnPtY, sy) = TwoFields(_pnlSpawn, "X", "Y", sy, pw, "Position");

        int spHalf = (pw - 4) / 2;
        var btnSpawnApply = DarkBtn("Apply",  4,              sy, spHalf);
        _btnSpawnDel      = DarkBtn("Delete", 4 + spHalf + 4, sy, spHalf, Color.FromArgb(130, 55, 55));
        sy += 30;

        btnSpawnApply.Click  += ApplySpawnSettings;
        _btnSpawnDel.Click   += (_, _) => _canvas.DeleteSelected();
        _pnlSpawn.Controls.AddRange(new Control[] { btnSpawnApply, _btnSpawnDel });
        _pnlSpawn.Height = sy;

        // ── Selected Platform ─────────────────────────────────────────────────
        _pnlPlatform = new Panel { Location = new(0, dynamicY), Width = panel.Width, AutoSize = false, Visible = false };
        panel.Controls.Add(_pnlPlatform);

        int py = 0;
        _pnlPlatform.Controls.Add(SectionLabel("SELECTED PLATFORM", 4, py, pw)); py += 22;
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

        // ── Selected Enemy ────────────────────────────────────────────────────
        _pnlEnemy = new Panel { Location = new(0, dynamicY), Width = panel.Width, AutoSize = false, Visible = false };
        panel.Controls.Add(_pnlEnemy);

        int ey = 0;
        _pnlEnemy.Controls.Add(SectionLabel("SELECTED ENEMY", 4, ey, pw)); ey += 22;
        (_txtEnemyX, _txtEnemyY, ey) = TwoFields(_pnlEnemy, "X", "Y", ey, pw, "Position");
        (_txtEnemyW, _txtEnemyH, ey) = TwoFields(_pnlEnemy, "Width", "Height", ey, pw, "Size");
        (_txtWaypointAX, _txtWaypointAY, ey) = TwoFields(_pnlEnemy, "X", "Y", ey, pw, "Waypoint A");
        (_txtWaypointBX, _txtWaypointBY, ey) = TwoFields(_pnlEnemy, "X", "Y", ey, pw, "Waypoint B");
        (_txtEnemySpeed, _txtEnemyDamage, _txtEnemyHP, ey) = ThreeFields(_pnlEnemy, "Speed", "Dmg", "HP", ey, pw, "Stats");

        int ehalf = (pw - 4) / 2;
        var btnEnemyApply = DarkBtn("Apply",  4,              ey, ehalf);
        var btnEnemyDel   = DarkBtn("Delete", 4 + ehalf + 4,  ey, ehalf, Color.FromArgb(130, 55, 55));
        ey += 30;

        btnEnemyApply.Click += ApplyEnemySettings;
        btnEnemyDel.Click   += (_, _) => _canvas.DeleteSelected();
        _pnlEnemy.Controls.AddRange(new Control[] { btnEnemyApply, btnEnemyDel });
        _pnlEnemy.Height  = ey;

        // ── Selected Ability Pickup ───────────────────────────────────────────
        _pnlPickup = new Panel { Location = new(0, dynamicY), Width = panel.Width, AutoSize = false, Visible = false };
        panel.Controls.Add(_pnlPickup);

        int pky = 0;
        _pnlPickup.Controls.Add(SectionLabel("SELECTED ABILITY PICKUP", 4, pky, pw)); pky += 22;
        (_, _txtPickupId, pky) = SingleField(_pnlPickup, "ID", pky, pw);

        _pnlPickup.Controls.Add(new Label
        {
            Text = "Ability:", ForeColor = Color.Silver,
            Font = new Font("Segoe UI", 7.5f),
            Location = new(4, pky), Size = new(pw, 15), AutoSize = false
        });
        pky += 15;
        _cboPickupAbility = new ComboBox
        {
            Location      = new(4, pky),
            Size          = new(pw, 22),
            DropDownStyle = ComboBoxStyle.DropDownList,
            BackColor     = Color.FromArgb(58, 58, 65),
            ForeColor     = Color.WhiteSmoke,
            FlatStyle     = FlatStyle.Flat,
            Font          = new Font("Consolas", 8.5f)
        };
        _cboPickupAbility.Items.AddRange(new object[] { "DoubleJump", "WallSlide", "Dash" });
        _cboPickupAbility.SelectedIndex = 0;
        _pnlPickup.Controls.Add(_cboPickupAbility);
        pky += 26;

        (_txtPickupX, _txtPickupY, pky) = TwoFields(_pnlPickup, "X", "Y", pky, pw, "Position");
        (_txtPickupW, _txtPickupH, pky) = TwoFields(_pnlPickup, "Width", "Height", pky, pw, "Size");

        int pkhalf = (pw - 4) / 2;
        var btnPickupApply = DarkBtn("Apply",  4,               pky, pkhalf);
        var btnPickupDel   = DarkBtn("Delete", 4 + pkhalf + 4,  pky, pkhalf, Color.FromArgb(130, 55, 55));
        pky += 30;

        btnPickupApply.Click += ApplyPickupSettings;
        btnPickupDel.Click   += (_, _) => _canvas.DeleteSelected();
        _pnlPickup.Controls.AddRange(new Control[] { btnPickupApply, btnPickupDel });
        _pnlPickup.Height  = pky;

        // ── Maps panel (below dynamic panels) ────────────────────────────────
        // Position it below the highest dynamic panel bottom. We use a fixed
        // Y that is below the dynamic area — the parent panel scrolls.
        int mapsY = dynamicY + 320;
        panel.Controls.Add(new Label { Height = 1, Location = new(4, mapsY - 8), Width = pw, BackColor = Color.FromArgb(70, 70, 80) });
        panel.Controls.Add(SectionLabel("WORLD MAPS", 4, mapsY, pw)); mapsY += 22;

        _lstMaps = new ListBox
        {
            Location      = new(4, mapsY),
            Size          = new(pw, 100),
            BackColor     = Color.FromArgb(58, 58, 65),
            ForeColor     = Color.WhiteSmoke,
            BorderStyle   = BorderStyle.FixedSingle,
            Font          = new Font("Consolas", 8.5f),
            DrawMode      = DrawMode.OwnerDrawFixed,
            ItemHeight    = 18
        };
        _lstMaps.DrawItem           += OnMapsListDrawItem;
        _lstMaps.SelectedIndexChanged += OnMapsListSelected;
        panel.Controls.Add(_lstMaps);
        mapsY += 104;

        int btnThird = (pw - 8) / 3;
        _btnAddMap = DarkBtn("Add…",   4,                  mapsY, btnThird);
        _btnNewMap = DarkBtn("New",    4 + btnThird + 4,   mapsY, btnThird);
        _btnRemMap = DarkBtn("Remove", 4 + (btnThird + 4) * 2, mapsY, btnThird, Color.FromArgb(130, 55, 55));
        mapsY += 28;

        _btnAddMap.Click += (_, _) => AddMapToWorld();
        _btnNewMap.Click += (_, _) => NewMapInWorld();
        _btnRemMap.Click += (_, _) => RemoveMapFromWorld();
        panel.Controls.AddRange(new Control[] { _btnAddMap, _btnNewMap, _btnRemMap });

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
        _worldStructureDirty = false;
        _canvas.LoadMap(map);
        SyncMapFields(map);
        RefreshMapsList();
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

            var editorMap = new EditorMap
            {
                Map      = map,
                WorldX   = 0,
                WorldY   = 0,
                FilePath = dlg.FileName
            };
            _canvas.LoadWorld(new List<EditorMap> { editorMap });
            SyncMapFields(map);
            RefreshMapsList();

            // Auto-create a single-map world
            _worldData = new WorldData();
            _worldData.Maps.Add(new WorldMapEntry
            {
                Path = Path.GetFileName(dlg.FileName),
                X    = 0,
                Y    = 0
            });
            _worldFilePath = null;

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
        // Regenerate auto-transitions before saving (only applies when multiple maps are loaded)
        _canvas.RegenerateTransitions();
        // Make transition targetMap paths relative for the game runtime
        NormalizeTransitionPaths(_canvas.Map, MapsDir());
        WriteFileForMap(path, _canvas.Map);
        _isDirty = false;
        if (_canvas.ActiveMap != null)
            _canvas.ActiveMap.IsDirty = false;
        UpdateTitle();
        RefreshMapsList();
        SetStatus($"Saved: {Path.GetFileName(path)}");
    }

    private void WriteFileForMap(string path, MapData map)
    {
        try
        {
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

    /// <summary>
    /// Converts absolute targetMap paths in auto-generated transitions to
    /// relative paths (e.g., "maps/world_02.json") for the game runtime.
    /// </summary>
    private static void NormalizeTransitionPaths(MapData map, string mapsDir)
    {
        if (map.Transitions == null) return;
        // Resolve the project root (parent of the maps/ directory)
        string? projectRoot = Path.GetDirectoryName(mapsDir);
        if (projectRoot == null) return;

        foreach (var tr in map.Transitions)
        {
            if (string.IsNullOrEmpty(tr.TargetMap)) continue;
            if (Path.IsPathRooted(tr.TargetMap))
            {
                tr.TargetMap = Path.GetRelativePath(projectRoot, tr.TargetMap)
                                   .Replace('\\', '/');
            }
        }
    }

    // ── World file I/O ────────────────────────────────────────────────────────
    private void NewWorld()
    {
        if (!ConfirmDiscard()) return;
        _worldData     = new WorldData();
        _worldFilePath = null;
        _filePath      = null;
        _isDirty       = false;
        _worldStructureDirty = false;

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
        _canvas.LoadMap(map);
        SyncMapFields(map);
        RefreshMapsList();
        UpdateTitle();
        SetStatus("New empty world created.");
    }

    private void OpenWorld()
    {
        if (!ConfirmDiscard()) return;
        using var dlg = new OpenFileDialog
        {
            Title            = "Open World",
            Filter           = "World files (*.world.json)|*.world.json|All files (*.*)|*.*",
            InitialDirectory = MapsDir()
        };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        LoadWorldFile(dlg.FileName);
    }

    private void LoadWorldFile(string worldPath)
    {
        try
        {
            var json = File.ReadAllText(worldPath);
            var opts = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
            var world = JsonSerializer.Deserialize<WorldData>(json, opts)
                        ?? throw new InvalidDataException("Failed to parse world file.");

            string worldDir = Path.GetDirectoryName(worldPath) ?? "";

            var editorMaps = new List<EditorMap>();

            foreach (var entry in world.Maps)
            {
                string mapPath = Path.IsPathRooted(entry.Path)
                    ? entry.Path
                    : Path.GetFullPath(Path.Combine(worldDir, entry.Path));

                if (!File.Exists(mapPath))
                {
                    MessageBox.Show(this,
                        $"Map file not found: {entry.Path}\nResolved to: {mapPath}",
                        "Missing Map", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    continue;
                }

                var mapJson = File.ReadAllText(mapPath);
                var map = JsonSerializer.Deserialize<MapData>(mapJson, opts);
                if (map == null) continue;

                editorMaps.Add(new EditorMap
                {
                    Map      = map,
                    WorldX   = entry.X,
                    WorldY   = entry.Y,
                    FilePath = mapPath
                });
            }

            if (editorMaps.Count == 0)
                throw new InvalidDataException("No valid maps found in world file.");

            _worldData     = world;
            _worldFilePath = worldPath;
            _filePath      = editorMaps[0].FilePath;
            _isDirty       = false;
            _worldStructureDirty = false;

            _canvas.LoadWorld(editorMaps);
            SyncMapFields(editorMaps[0].Map);
            RefreshMapsList();
            UpdateTitle();
            SetStatus($"Opened world: {Path.GetFileName(worldPath)} ({editorMaps.Count} map(s))");
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, $"Could not open world file:\n{ex.Message}",
                "Open Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void SaveWorld()
    {
        if (_worldData == null) { Warn("No world is loaded."); return; }
        if (_worldFilePath == null) { SaveWorldAs(); return; }
        WriteWorldFile(_worldFilePath);
    }

    private void SaveWorldAs()
    {
        if (_worldData == null) { Warn("No world is loaded."); return; }
        using var dlg = new SaveFileDialog
        {
            Title            = "Save World",
            Filter           = "World files (*.world.json)|*.world.json|All files (*.*)|*.*",
            FileName         = _worldFilePath != null
                ? Path.GetFileName(_worldFilePath)
                : "my_world.world.json",
            InitialDirectory = MapsDir()
        };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;
        _worldFilePath = dlg.FileName;
        WriteWorldFile(_worldFilePath);
    }

    private void WriteWorldFile(string worldPath)
    {
        if (_worldData == null || _canvas.Map == null) return;
        try
        {
            string worldDir = Path.GetDirectoryName(worldPath) ?? "";

            // Regenerate auto-transitions before saving
            _canvas.RegenerateTransitions();

            // Resolve the project maps/ directory for relative targetMap paths
            string mapsDir = MapsDir();

            // Save each map file from the canvas EditorMaps
            foreach (var em in _canvas.EditorMaps)
            {
                if (!string.IsNullOrEmpty(em.FilePath))
                {
                    // Make transition targetMap paths relative to the project root
                    // (e.g., "maps/world_02.json") for the game runtime's MapLoader
                    NormalizeTransitionPaths(em.Map, mapsDir);
                    WriteFileForMap(em.FilePath, em.Map);
                }
            }

            // Write the world file with paths relative to the world file's directory
            var worldToSave = new WorldData();
            foreach (var em in _canvas.EditorMaps)
            {
                string relativePath = em.FilePath;
                if (Path.IsPathRooted(relativePath) && !string.IsNullOrEmpty(worldDir))
                {
                    relativePath = Path.GetRelativePath(worldDir, relativePath);
                }
                worldToSave.Maps.Add(new WorldMapEntry
                {
                    Path = relativePath.Replace('\\', '/'),
                    X    = em.WorldX,
                    Y    = em.WorldY
                });
            }

            var opts = new JsonSerializerOptions
            {
                WriteIndented = true,
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
            };
            var worldJson = JsonSerializer.Serialize(worldToSave, opts);
            File.WriteAllText(worldPath, worldJson);

            _isDirty = false;
            _worldStructureDirty = false;
            foreach (var em in _canvas.EditorMaps)
                em.IsDirty = false;
            UpdateTitle();
            RefreshMapsList();
            SetStatus($"World saved: {Path.GetFileName(worldPath)}");
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, $"Could not save world file:\n{ex.Message}",
                "Save Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
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
            MarkDirty();
            _canvas.RegenerateTransitions();
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

    private void ApplyEnemySettings(object? sender, EventArgs e)
    {
        var en = _canvas.SelectedEnemy;
        if (en == null) return;
        try
        {
            en.X               = ParseF(_txtEnemyX);
            en.Y               = ParseF(_txtEnemyY);
            en.Width           = MathF.Max(1f, ParseF(_txtEnemyW));
            en.Height          = MathF.Max(1f, ParseF(_txtEnemyH));
            en.WaypointA.X     = ParseF(_txtWaypointAX);
            en.WaypointA.Y     = ParseF(_txtWaypointAY);
            en.WaypointB.X     = ParseF(_txtWaypointBX);
            en.WaypointB.Y     = ParseF(_txtWaypointBY);
            en.Speed           = ParseF(_txtEnemySpeed);
            en.Damage          = ParseF(_txtEnemyDamage);
            en.Hp              = ParseF(_txtEnemyHP);
            MarkDirty();
            _canvas.Invalidate();
            SetStatus("Enemy updated.");
        }
        catch { Warn("Invalid values in enemy settings. Use numeric values."); }
    }

    private void ApplyPickupSettings(object? sender, EventArgs e)
    {
        var pk = _canvas.SelectedPickup;
        if (pk == null) return;
        try
        {
            pk.Id      = _txtPickupId.Text.Trim();
            pk.Ability = _cboPickupAbility.SelectedItem?.ToString() ?? "DoubleJump";
            pk.X       = ParseF(_txtPickupX);
            pk.Y       = ParseF(_txtPickupY);
            pk.Width   = MathF.Max(1f, ParseF(_txtPickupW));
            pk.Height  = MathF.Max(1f, ParseF(_txtPickupH));
            MarkDirty();
            _canvas.Invalidate();
            SetStatus("Ability pickup updated.");
        }
        catch { Warn("Invalid values in pickup settings. Use numeric values for position/size."); }
    }

    // ── Spawn point handlers ──────────────────────────────────────────────────
    private void OnSpawnListSelected(object? sender, EventArgs e)
    {
        if (_syncingUI) return;
        if (_lstSpawnPoints.SelectedItem is not string key) return;
        // Sync canvas selection with list
        if (key == "(default)")
            _canvas.SelectSpawnByKey("");
        else
            _canvas.SelectSpawnByKey(key);
    }

    private void ApplySpawnSettings(object? sender, EventArgs e)
    {
        var map = _canvas.Map;
        if (map == null) return;

        if (_canvas.SelectedDefaultSpawn)
        {
            try
            {
                map.SpawnPoint.X = ParseF(_txtSpawnPtX);
                map.SpawnPoint.Y = ParseF(_txtSpawnPtY);
                MarkDirty();
                _canvas.Invalidate();
                SetStatus("Default spawn point updated.");
            }
            catch { Warn("Invalid X/Y values. Use numeric values."); }
        }
        else if (_canvas.SelectedSpawnKey != null)
        {
            var newName = _txtSpawnName.Text.Trim();
            if (string.IsNullOrEmpty(newName))
            {
                Warn("Spawn point name cannot be empty.");
                return;
            }
            var oldKey = _canvas.SelectedSpawnKey;
            if (newName != oldKey && map.SpawnPoints.ContainsKey(newName))
            {
                Warn($"A spawn point named \"{newName}\" already exists.");
                return;
            }
            try
            {
                var pt = new PointData { X = ParseF(_txtSpawnPtX), Y = ParseF(_txtSpawnPtY) };
                if (newName != oldKey) map.SpawnPoints.Remove(oldKey);
                map.SpawnPoints[newName] = pt;
                MarkDirty();
                // Re-select with new name if renamed
                if (newName != oldKey)
                    _canvas.SelectSpawnByKey(newName);
                else
                    _canvas.Invalidate();
                RefreshSpawnPointsList();
                SetStatus($"Spawn point \"{newName}\" updated.");
            }
            catch { Warn("Invalid X/Y values. Use numeric values."); }
        }
    }

    private void RefreshSpawnPointsList()
    {
        var map = _canvas.Map;
        _lstSpawnPoints.Items.Clear();
        if (map == null) return;
        _lstSpawnPoints.Items.Add("(default)");
        foreach (var key in map.SpawnPoints.Keys)
            _lstSpawnPoints.Items.Add(key);
    }

    private void ShowDynamicPanel(Panel? toShow)
    {
        _pnlSpawn.Visible      = toShow == _pnlSpawn;
        _pnlPlatform.Visible   = toShow == _pnlPlatform;
        _pnlEnemy.Visible      = toShow == _pnlEnemy;
        _pnlPickup.Visible     = toShow == _pnlPickup;
    }

    // ── Canvas event handlers ─────────────────────────────────────────────────
    private void OnSelectionChanged(object? sender, EventArgs e)
    {
        var selType = _canvas.SelectedType;

        switch (selType)
        {
            case SelectableType.Platform:
                ShowDynamicPanel(_pnlPlatform);
                var p = _canvas.SelectedPlatform!;
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
                break;

            case SelectableType.Enemy:
                ShowDynamicPanel(_pnlEnemy);
                var en = _canvas.SelectedEnemy!;
                _syncingUI = true;
                _txtEnemyX.Text       = en.X.ToString("F1");
                _txtEnemyY.Text       = en.Y.ToString("F1");
                _txtEnemyW.Text       = en.Width.ToString("F1");
                _txtEnemyH.Text       = en.Height.ToString("F1");
                _txtWaypointAX.Text   = en.WaypointA.X.ToString("F1");
                _txtWaypointAY.Text   = en.WaypointA.Y.ToString("F1");
                _txtWaypointBX.Text   = en.WaypointB.X.ToString("F1");
                _txtWaypointBY.Text   = en.WaypointB.Y.ToString("F1");
                _txtEnemySpeed.Text   = en.Speed.ToString("F1");
                _txtEnemyDamage.Text  = en.Damage.ToString("F1");
                _txtEnemyHP.Text      = en.Hp.ToString("F1");
                _syncingUI = false;
                break;

            case SelectableType.Pickup:
                ShowDynamicPanel(_pnlPickup);
                var pk = _canvas.SelectedPickup!;
                _syncingUI = true;
                _txtPickupId.Text = pk.Id;
                int idx = _cboPickupAbility.Items.IndexOf(pk.Ability);
                _cboPickupAbility.SelectedIndex = idx >= 0 ? idx : 0;
                _txtPickupX.Text = pk.X.ToString("F1");
                _txtPickupY.Text = pk.Y.ToString("F1");
                _txtPickupW.Text = pk.Width.ToString("F1");
                _txtPickupH.Text = pk.Height.ToString("F1");
                _syncingUI = false;
                break;

            case SelectableType.DefaultSpawn:
                ShowDynamicPanel(_pnlSpawn);
                var dsp = _canvas.Map!.SpawnPoint;
                _syncingUI = true;
                _lblSpawnTitle.Text = "DEFAULT SPAWN POINT";
                _txtSpawnName.Text    = "default";
                _txtSpawnName.Enabled = false;
                _btnSpawnDel.Enabled  = false;
                _txtSpawnPtX.Text     = dsp.X.ToString("F1");
                _txtSpawnPtY.Text     = dsp.Y.ToString("F1");
                RefreshSpawnPointsList();
                _lstSpawnPoints.SelectedItem = "(default)";
                _syncingUI = false;
                break;

            case SelectableType.NamedSpawn:
                ShowDynamicPanel(_pnlSpawn);
                var spKey = _canvas.SelectedSpawnKey!;
                var nsp = _canvas.Map!.SpawnPoints[spKey];
                _syncingUI = true;
                _lblSpawnTitle.Text = "NAMED SPAWN POINT";
                _txtSpawnName.Text    = spKey;
                _txtSpawnName.Enabled = true;
                _btnSpawnDel.Enabled  = true;
                _txtSpawnPtX.Text     = nsp.X.ToString("F1");
                _txtSpawnPtY.Text     = nsp.Y.ToString("F1");
                RefreshSpawnPointsList();
                _lstSpawnPoints.SelectedItem = spKey;
                _syncingUI = false;
                break;

            default:
                ShowDynamicPanel(null);
                break;
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
        var en = _canvas.SelectedEnemy;
        if (en != null && !_syncingUI)
        {
            _syncingUI = true;
            _txtEnemyX.Text       = en.X.ToString("F1");
            _txtEnemyY.Text       = en.Y.ToString("F1");
            _txtEnemyW.Text       = en.Width.ToString("F1");
            _txtEnemyH.Text       = en.Height.ToString("F1");
            _txtWaypointAX.Text   = en.WaypointA.X.ToString("F1");
            _txtWaypointAY.Text   = en.WaypointA.Y.ToString("F1");
            _txtWaypointBX.Text   = en.WaypointB.X.ToString("F1");
            _txtWaypointBY.Text   = en.WaypointB.Y.ToString("F1");
            _syncingUI = false;
        }
        var pk = _canvas.SelectedPickup;
        if (pk != null && !_syncingUI)
        {
            _syncingUI = true;
            _txtPickupX.Text = pk.X.ToString("F1");
            _txtPickupY.Text = pk.Y.ToString("F1");
            _txtPickupW.Text = pk.Width.ToString("F1");
            _txtPickupH.Text = pk.Height.ToString("F1");
            _syncingUI = false;
        }
        if (_canvas.SelectedDefaultSpawn && !_syncingUI)
        {
            var sp = _canvas.Map!.SpawnPoint;
            _syncingUI = true;
            _txtSpawnPtX.Text = sp.X.ToString("F1");
            _txtSpawnPtY.Text = sp.Y.ToString("F1");
            _syncingUI = false;
        }
        if (_canvas.SelectedSpawnKey != null && !_syncingUI)
        {
            if (_canvas.Map!.SpawnPoints.TryGetValue(_canvas.SelectedSpawnKey, out var nsp))
            {
                _syncingUI = true;
                _txtSpawnPtX.Text = nsp.X.ToString("F1");
                _txtSpawnPtY.Text = nsp.Y.ToString("F1");
                _syncingUI = false;
            }
        }
        // Refresh spawn list if a spawn was added/removed
        _syncingUI = true;
        RefreshSpawnPointsList();
        _syncingUI = false;
        MarkDirty();
    }

    private void OnZoomChanged(object? sender, EventArgs e)
    {
        _lblZoom.Text = $"Zoom: {_canvas.Zoom * 100:F0}%";
    }

    private void OnActiveMapChanged(object? sender, EventArgs e)
    {
        // Sync the map settings panel with the new active map
        if (_canvas.Map != null)
            SyncMapFields(_canvas.Map);
        RefreshMapsList();
        UpdateStatus();
    }

    // ── Tool helpers ──────────────────────────────────────────────────────────
    private void SetTool(EditorTool t)
    {
        _canvas.Tool               = t;
        _btnSelect.Checked         = t == EditorTool.Select;
        _btnDraw.Checked           = t == EditorTool.Draw;
        _btnDrawEnemy.Checked      = t == EditorTool.DrawEnemy;
        _btnDrawPickup.Checked     = t == EditorTool.DrawPickup;
        _btnDrawSpawn.Checked      = t == EditorTool.DrawSpawnPoint;
        _btnMoveMap.Checked        = t == EditorTool.MoveMap;
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
        string mapName = _canvas.ActiveMap?.Map.Name ?? "";
        int    platCount   = _canvas.Map?.Platforms.Count ?? 0;
        int    enemyCount  = _canvas.Map?.Enemies?.Count ?? 0;
        int    transCount  = _canvas.Map?.Transitions?.Count ?? 0;
        int    pickupCount = _canvas.Map?.AbilityPickups?.Count ?? 0;
        int    spawnCount  = ((_canvas.Map?.SpawnPoints?.Count ?? 0) + 1);
        string tool       = _canvas.Tool switch
        {
            EditorTool.Draw           => "Draw Platform",
            EditorTool.DrawEnemy      => "Draw Enemy",
            EditorTool.DrawPickup     => "Draw Pickup",
            EditorTool.DrawSpawnPoint => "Draw Spawn",
            EditorTool.MoveMap        => "Move Map",
            _                         => "Select"
        };
        string sel = _canvas.SelectedPlatform    != null ? " | Platform selected"     :
                     _canvas.SelectedEnemy       != null ? " | Enemy selected"        :
                     _canvas.SelectedPickup      != null ? " | Pickup selected"       :
                     _canvas.SelectedDefaultSpawn        ? " | Default spawn selected" :
                     _canvas.SelectedSpawnKey     != null ? $" | Spawn \"{_canvas.SelectedSpawnKey}\" selected" : "";
        string active = !string.IsNullOrEmpty(mapName) ? $"Active: {mapName} | " : "";
        _lblInfo.Text = $"{active}{platCount} platforms, {enemyCount} enemies, {transCount} transitions, {pickupCount} pickups, {spawnCount} spawns{sel} | {tool} mode";
    }

    private void SetStatus(string msg) => _lblInfo.Text = msg;

    private void MarkDirty()
    {
        _isDirty = true;
        if (_canvas.ActiveMap != null)
            _canvas.ActiveMap.IsDirty = true;
        UpdateTitle();
        UpdateStatus();
    }

    private void UpdateTitle()
    {
        string name;
        if (_worldFilePath != null)
            name = Path.GetFileName(_worldFilePath);
        else if (_filePath != null)
            name = Path.GetFileName(_filePath);
        else
            name = "Untitled";

        bool anyDirty = _isDirty || _worldStructureDirty;
        foreach (var em in _canvas.EditorMaps)
            if (em.IsDirty) anyDirty = true;

        Text = $"Map Editor — {name}{(anyDirty ? " *" : "")}";
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
        _syncingUI = false;
        RefreshSpawnPointsList();
        ShowDynamicPanel(null);
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
            if (e.Shift)
            {
                switch (e.KeyCode)
                {
                    case Keys.N: NewWorld();   e.Handled = true; return;
                    case Keys.O: OpenWorld();  e.Handled = true; return;
                    case Keys.S: SaveWorld();  e.Handled = true; return;
                }
            }
            switch (e.KeyCode)
            {
                case Keys.N: NewMap();    e.Handled = true; return;
                case Keys.O: OpenFile();  e.Handled = true; return;
                case Keys.S: SaveFile();  e.Handled = true; return;
            }
        }
        switch (e.KeyCode)
        {
            case Keys.S: SetTool(EditorTool.Select);         e.Handled = true; break;
            case Keys.D: SetTool(EditorTool.Draw);           e.Handled = true; break;
            case Keys.E: SetTool(EditorTool.DrawEnemy);      e.Handled = true; break;
            case Keys.P: SetTool(EditorTool.DrawPickup);     e.Handled = true; break;
            case Keys.W: SetTool(EditorTool.DrawSpawnPoint); e.Handled = true; break;
            case Keys.M: SetTool(EditorTool.MoveMap);       e.Handled = true; break;
            case Keys.F: _canvas.FitToView();         e.Handled = true; break;
            case Keys.G: ToggleSnap();                e.Handled = true; break;
            case Keys.D1: _canvas.SetZoom(1f);        e.Handled = true; break;
            case Keys.OemMinus: _canvas.SetZoom(_canvas.Zoom / 1.25f); e.Handled = true; break;
            case Keys.Oemplus:  _canvas.SetZoom(_canvas.Zoom * 1.25f); e.Handled = true; break;
        }
    }

    // ── World management UI ──────────────────────────────────────────────────
    private void RefreshMapsList()
    {
        _syncingUI = true;
        _lstMaps.Items.Clear();
        foreach (var em in _canvas.EditorMaps)
            _lstMaps.Items.Add(em);
        if (_canvas.ActiveMap != null)
            _lstMaps.SelectedItem = _canvas.ActiveMap;
        _syncingUI = false;
    }

    private void OnMapsListDrawItem(object? sender, DrawItemEventArgs e)
    {
        if (e.Index < 0) return;
        e.DrawBackground();
        var em = (EditorMap)_lstMaps.Items[e.Index];
        bool isActive = em == _canvas.ActiveMap;
        string label = em.Map.Name;
        if (!string.IsNullOrEmpty(em.FilePath))
            label += $"  ({Path.GetFileName(em.FilePath)})";
        if (em.IsDirty)
            label += " *";

        var font = isActive
            ? new Font(e.Font ?? _lstMaps.Font, FontStyle.Bold)
            : e.Font ?? _lstMaps.Font;
        var brush = (e.State & DrawItemState.Selected) != 0
            ? SystemBrushes.HighlightText
            : (isActive ? Brushes.Cyan : Brushes.WhiteSmoke);

        e.Graphics.DrawString(label, font, brush, e.Bounds.X + 2, e.Bounds.Y + 1);
        if (isActive) font.Dispose();
        e.DrawFocusRectangle();
    }

    private void OnMapsListSelected(object? sender, EventArgs e)
    {
        if (_syncingUI) return;
        if (_lstMaps.SelectedItem is EditorMap em)
            _canvas.SetActiveMap(em);
    }

    private void AddMapToWorld()
    {
        using var dlg = new OpenFileDialog
        {
            Title            = "Add Map to World",
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

            // Default position: right of the rightmost existing map
            float newX = 0, newY = 0;
            if (_canvas.EditorMaps.Count > 0)
            {
                float maxRight = float.MinValue;
                float topY = float.MaxValue;
                foreach (var em in _canvas.EditorMaps)
                {
                    float right = em.WorldX + em.Map.Bounds.X + em.Map.Bounds.Width;
                    if (right > maxRight) { maxRight = right; topY = em.WorldY; }
                }
                newX = maxRight - map.Bounds.X;
                newY = topY;
            }

            var editorMap = new EditorMap
            {
                Map      = map,
                WorldX   = newX,
                WorldY   = newY,
                FilePath = dlg.FileName
            };
            _canvas.EditorMaps.Add(editorMap);
            _canvas.SetActiveMap(editorMap);

            // Ensure we have a world data model
            if (_worldData == null)
                _worldData = new WorldData();

            MarkWorldStructureDirty();
            _canvas.RegenerateTransitions();
            _canvas.FitToView();
            RefreshMapsList();
            SyncMapFields(map);
            SetStatus($"Added map: {map.Name}");
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, $"Could not add map:\n{ex.Message}",
                "Add Map Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void NewMapInWorld()
    {
        using var dlg = new SaveFileDialog
        {
            Title            = "Save New Map As",
            Filter           = "JSON map files (*.json)|*.json|All files (*.*)|*.*",
            FileName         = "new_map.json",
            InitialDirectory = MapsDir()
        };
        if (dlg.ShowDialog(this) != DialogResult.OK) return;

        var map = new MapData
        {
            Name       = Path.GetFileNameWithoutExtension(dlg.FileName),
            Bounds     = new() { X = -200, Y = -500, Width = 3600, Height = 1200 },
            SpawnPoint = new() { X = 150, Y = 475 },
            Platforms  = new()
            {
                new() { X = -200, Y = 500, Width = 3600, Height = 40, R = 80, G = 80, B = 80 }
            }
        };

        // Save the new map file immediately
        WriteFileForMap(dlg.FileName, map);

        // Default position: right of the rightmost existing map
        float newX = 0, newY = 0;
        if (_canvas.EditorMaps.Count > 0)
        {
            float maxRight = float.MinValue;
            float topY = float.MaxValue;
            foreach (var em in _canvas.EditorMaps)
            {
                float right = em.WorldX + em.Map.Bounds.X + em.Map.Bounds.Width;
                if (right > maxRight) { maxRight = right; topY = em.WorldY; }
            }
            newX = maxRight - map.Bounds.X;
            newY = topY;
        }

        var editorMap = new EditorMap
        {
            Map      = map,
            WorldX   = newX,
            WorldY   = newY,
            FilePath = dlg.FileName
        };
        _canvas.EditorMaps.Add(editorMap);
        _canvas.SetActiveMap(editorMap);

        if (_worldData == null)
            _worldData = new WorldData();

        MarkWorldStructureDirty();
        _canvas.RegenerateTransitions();
        _canvas.FitToView();
        RefreshMapsList();
        SyncMapFields(map);
        SetStatus($"Created new map: {map.Name}");
    }

    private void RemoveMapFromWorld()
    {
        var active = _canvas.ActiveMap;
        if (active == null) { Warn("No active map to remove."); return; }

        if (active.IsDirty)
        {
            var result = MessageBox.Show(this,
                $"Map \"{active.Map.Name}\" has unsaved changes.\nRemove it from the world anyway?",
                "Unsaved Changes", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            if (result != DialogResult.Yes) return;
        }

        _canvas.EditorMaps.Remove(active);

        // Switch to the next available map
        if (_canvas.EditorMaps.Count > 0)
            _canvas.SetActiveMap(_canvas.EditorMaps[0]);
        else
            _canvas.SetActiveMap(null);

        MarkWorldStructureDirty();
        _canvas.RegenerateTransitions();
        _canvas.FitToView();
        RefreshMapsList();

        if (_canvas.Map != null)
            SyncMapFields(_canvas.Map);
        SetStatus($"Removed map: {active.Map.Name}");
    }

    private void MarkWorldStructureDirty()
    {
        _worldStructureDirty = true;
        MarkDirty();
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
        bool anyDirty = _isDirty || _worldStructureDirty;
        foreach (var em in _canvas.EditorMaps)
            if (em.IsDirty) anyDirty = true;
        if (!anyDirty) return true;
        return MessageBox.Show(this,
            "You have unsaved changes. Discard them?", "Unsaved Changes",
            MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes;
    }

    private static float ParseF(TextBox t) => float.Parse(t.Text.Trim());
    private static int   ParseI(TextBox t) => int.Parse(t.Text.Trim());
    private void Warn(string msg) =>
        MessageBox.Show(this, msg, "Invalid Input", MessageBoxButtons.OK, MessageBoxIcon.Warning);
}
