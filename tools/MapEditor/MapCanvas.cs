using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace MapEditor;

public enum EditorTool { Select, Draw, DrawEnemy, DrawTransition, DrawPickup, DrawSpawnPoint }
public enum SelectableType { None, Platform, Enemy, Transition, Pickup, DefaultSpawn, NamedSpawn }
internal enum ResizeHandle { None, Move, N, NE, E, SE, S, SW, W, NW }

public sealed class MapCanvas : Control
{
    // ── Viewport ──────────────────────────────────────────────────────────────
    private float _zoom = 0.2f;
    private float _panX = 0f;   // world X of the canvas left edge
    private float _panY = 0f;   // world Y of the canvas top  edge

    // ── Data ──────────────────────────────────────────────────────────────────
    public MapData? Map { get; private set; }

    // ── Selection ─────────────────────────────────────────────────────────────
    private PlatformData? _selected;
    public  PlatformData? SelectedPlatform => _selected;

    private EnemyData? _selectedEnemy;
    public  EnemyData? SelectedEnemy => _selectedEnemy;

    private TransitionData? _selectedTransition;
    public  TransitionData? SelectedTransition => _selectedTransition;

    private AbilityPickupData? _selectedPickup;
    public  AbilityPickupData? SelectedPickup => _selectedPickup;

    private bool _selectedDefaultSpawn;
    public  bool SelectedDefaultSpawn => _selectedDefaultSpawn;

    private string? _selectedSpawnKey;
    public  string? SelectedSpawnKey => _selectedSpawnKey;

    public  SelectableType SelectedType =>
        _selected != null ? SelectableType.Platform :
        _selectedEnemy != null ? SelectableType.Enemy :
        _selectedTransition != null ? SelectableType.Transition :
        _selectedPickup != null ? SelectableType.Pickup :
        _selectedDefaultSpawn ? SelectableType.DefaultSpawn :
        _selectedSpawnKey != null ? SelectableType.NamedSpawn :
        SelectableType.None;

    // ── Tool ──────────────────────────────────────────────────────────────────
    private EditorTool _tool = EditorTool.Select;
    public  EditorTool Tool
    {
        get => _tool;
        set { _tool = value; UpdateCursorForTool(); Invalidate(); }
    }

    public bool  SnapToGrid { get; set; } = true;
    public float GridSize   { get; set; } = 10f;

    // ── Events ────────────────────────────────────────────────────────────────
    public event EventHandler? SelectionChanged;
    public event EventHandler? MapChanged;
    public event EventHandler? ZoomChanged;

    // ── Drag state ────────────────────────────────────────────────────────────
    private bool         _isPanning;
    private bool         _isDrawing;
    private bool         _isDragging;
    private Point        _lastScreen;
    private PointF       _dragStartWorld;
    private RectangleF   _drawRect;
    private ResizeHandle _activeHandle;
    private RectangleF   _origRect;       // platform rect at drag start
    private float        _moveOffX, _moveOffY;
    private PointF       _origWaypointA, _origWaypointB;

    // ── Appearance ────────────────────────────────────────────────────────────
    private const int HandlePx = 8;
    private static readonly Color BackCol         = Color.FromArgb(35, 35, 40);
    private static readonly Color BoundsCol       = Color.FromArgb(50, 50, 58);
    private static readonly Color DeathZoneCol    = Color.FromArgb(55, 110, 25, 25);
    private static readonly Color GridMinorCol    = Color.FromArgb(22, 255, 255, 255);
    private static readonly Color GridMajorCol    = Color.FromArgb(44, 255, 255, 255);
    private static readonly Color BoundsBorderCol = Color.FromArgb(120, 80, 80, 200);

    // ── Constructor ───────────────────────────────────────────────────────────
    public MapCanvas()
    {
        SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint |
                 ControlStyles.DoubleBuffer         | ControlStyles.ResizeRedraw, true);
        BackColor = BackCol;
    }

    // ── Map management ────────────────────────────────────────────────────────
    public void LoadMap(MapData map)
    {
        Map            = map;
        _selected      = null;
        _selectedEnemy = null;
        _selectedTransition = null;
        _selectedPickup = null;
        _selectedDefaultSpawn = false;
        _selectedSpawnKey = null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        FitToView();
    }

    public void FitToView()
    {
        if (Map == null || Width < 10 || Height < 10) return;
        var b    = Map.Bounds;
        float zx = Width  / b.Width;
        float zy = Height / b.Height;
        _zoom = Math.Min(zx, zy) * 0.88f;
        _panX = b.X + b.Width  / 2f - Width  / (2f * _zoom);
        _panY = b.Y + b.Height / 2f - Height / (2f * _zoom);
        ZoomChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    public void SetZoom(float zoom)
    {
        float cx = _panX + Width  / (2f * _zoom);
        float cy = _panY + Height / (2f * _zoom);
        _zoom = Math.Clamp(zoom, 0.03f, 12f);
        _panX = cx - Width  / (2f * _zoom);
        _panY = cy - Height / (2f * _zoom);
        ZoomChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    public float  Zoom => _zoom;
    public PointF WorldAt(Point screen) => S2W(screen);

    // ── Coordinate helpers ────────────────────────────────────────────────────
    private PointF     S2W(Point p)                        => new(p.X / _zoom + _panX, p.Y / _zoom + _panY);
    private PointF     W2S(float wx, float wy)             => new((wx - _panX) * _zoom, (wy - _panY) * _zoom);
    private RectangleF WR2S(float x, float y, float w, float h)
        => new((x - _panX) * _zoom, (y - _panY) * _zoom, w * _zoom, h * _zoom);
    private float Snap(float v) => SnapToGrid ? MathF.Round(v / GridSize) * GridSize : v;

    // ── Paint ─────────────────────────────────────────────────────────────────
    protected override void OnPaint(PaintEventArgs e)
    {
        var g = e.Graphics;
        g.SmoothingMode = SmoothingMode.None;
        g.Clear(BackCol);

        if (Map == null) { DrawNoMap(g); return; }

        DrawDeathZone(g);
        DrawBounds(g);
        DrawGrid(g);
        DrawPlatforms(g);
        DrawEnemies(g);
        DrawTransitions(g);
        DrawPickups(g);
        if (_isDrawing)       DrawGhost(g);
        if (_selected != null) { DrawSelectionOutline(g, _selected.X, _selected.Y, _selected.Width, _selected.Height); DrawSelectionHandles(g, _selected.X, _selected.Y, _selected.Width, _selected.Height); }
        if (_selectedEnemy != null) { DrawSelectionOutline(g, _selectedEnemy.X, _selectedEnemy.Y, _selectedEnemy.Width, _selectedEnemy.Height); DrawSelectionHandles(g, _selectedEnemy.X, _selectedEnemy.Y, _selectedEnemy.Width, _selectedEnemy.Height); }
        if (_selectedTransition != null) { DrawSelectionOutline(g, _selectedTransition.X, _selectedTransition.Y, _selectedTransition.Width, _selectedTransition.Height); DrawSelectionHandles(g, _selectedTransition.X, _selectedTransition.Y, _selectedTransition.Width, _selectedTransition.Height); }
        if (_selectedPickup != null) { DrawSelectionOutline(g, _selectedPickup.X, _selectedPickup.Y, _selectedPickup.Width, _selectedPickup.Height); DrawSelectionHandles(g, _selectedPickup.X, _selectedPickup.Y, _selectedPickup.Width, _selectedPickup.Height); }
        DrawSpawnPoint(g);
        DrawNamedSpawnPoints(g);
    }

    private void DrawNoMap(Graphics g)
    {
        using var f = new Font("Segoe UI", 14f);
        const string msg = "No map loaded — use File > New or File > Open";
        var sz = g.MeasureString(msg, f);
        g.DrawString(msg, f, Brushes.DimGray, (Width - sz.Width) / 2f, (Height - sz.Height) / 2f);
    }

    private void DrawDeathZone(Graphics g)
    {
        var b  = Map!.Bounds;
        var sr = WR2S(b.X, b.Y + b.Height, b.Width, Height / _zoom + 200f);
        using var br = new SolidBrush(DeathZoneCol);
        g.FillRectangle(br, sr);
        // Label
        if (_zoom > 0.1f)
        {
            using var f = new Font("Segoe UI", 8f);
            g.DrawString("DEATH ZONE", f, Brushes.IndianRed,
                (b.X - _panX) * _zoom + 4, (b.Y + b.Height - _panY) * _zoom + 4);
        }
    }

    private void DrawBounds(Graphics g)
    {
        var b  = Map!.Bounds;
        var sr = WR2S(b.X, b.Y, b.Width, b.Height);
        using var fill = new SolidBrush(BoundsCol);
        g.FillRectangle(fill, sr);
        using var pen = new Pen(BoundsBorderCol, 1.5f) { DashStyle = DashStyle.Dash };
        g.DrawRectangle(pen, sr.X, sr.Y, sr.Width, sr.Height);
    }

    private void DrawGrid(Graphics g)
    {
        if (_zoom < 0.10f) return;
        var b = Map!.Bounds;

        float minor = _zoom < 0.22f ? 50f : 10f;
        float major = 100f;

        using var minorPen = new Pen(GridMinorCol, 1f);
        using var majorPen = new Pen(GridMajorCol, 1f);

        float startX = MathF.Ceiling(b.X / minor) * minor;
        for (float wx = startX; wx <= b.X + b.Width; wx += minor)
        {
            var pen = (wx % major == 0) ? majorPen : minorPen;
            float sx = (wx - _panX) * _zoom;
            g.DrawLine(pen, sx, (b.Y - _panY) * _zoom, sx, (b.Y + b.Height - _panY) * _zoom);
        }

        float startY = MathF.Ceiling(b.Y / minor) * minor;
        for (float wy = startY; wy <= b.Y + b.Height; wy += minor)
        {
            var pen = (wy % major == 0) ? majorPen : minorPen;
            float sy = (wy - _panY) * _zoom;
            g.DrawLine(pen, (b.X - _panX) * _zoom, sy, (b.X + b.Width - _panX) * _zoom, sy);
        }
    }

    private void DrawPlatforms(Graphics g)
    {
        foreach (var p in Map!.Platforms)
        {
            if (p == _selected) continue;
            DrawPlatformFill(g, p);
        }
        // Draw selected on top so it is not obscured
        if (_selected != null) DrawPlatformFill(g, _selected);
    }

    private void DrawPlatformFill(Graphics g, PlatformData p)
    {
        var sr = WR2S(p.X, p.Y, p.Width, p.Height);
        using var br   = new SolidBrush(Color.FromArgb(p.R, p.G, p.B));
        using var edge = new Pen(Color.FromArgb(60, 0, 0, 0), 1f);
        g.FillRectangle(br, sr);
        g.DrawRectangle(edge, sr.X, sr.Y, sr.Width, sr.Height);
    }

    private void DrawSelectionOutline(Graphics g, float wx, float wy, float ww, float wh)
    {
        var sr = WR2S(wx, wy, ww, wh);
        using var pen = new Pen(Color.Cyan, 2f);
        g.DrawRectangle(pen, sr.X - 1, sr.Y - 1, sr.Width + 2, sr.Height + 2);
    }

    private void DrawGhost(Graphics g)
    {
        var norm = Normalize(_drawRect);
        if (norm.Width < 1f || norm.Height < 1f) return;
        var sr = WR2S(norm.X, norm.Y, norm.Width, norm.Height);
        using var br  = new SolidBrush(Color.FromArgb(90, 80, 200, 80));
        using var pen = new Pen(Color.LightGreen, 1.5f);
        g.FillRectangle(br, sr);
        g.DrawRectangle(pen, sr.X, sr.Y, sr.Width, sr.Height);
        if (sr.Width > 50 && sr.Height > 16)
        {
            using var f = new Font("Consolas", 8f);
            g.DrawString($"{norm.Width:F0} × {norm.Height:F0}", f, Brushes.White, sr.X + 4, sr.Y + 3);
        }
    }

    private void DrawSelectionHandles(Graphics g, float wx, float wy, float ww, float wh)
    {
        foreach (var (_, rect) in GetHandleRects(wx, wy, ww, wh))
        {
            g.FillRectangle(Brushes.White, rect);
            g.DrawRectangle(Pens.SteelBlue, rect.X, rect.Y, rect.Width, rect.Height);
        }
    }

    private void DrawSpawnPoint(Graphics g)
    {
        var sp = Map!.SpawnPoint;
        var s  = W2S(sp.X, sp.Y);
        float r = Math.Max(5f, 8f * _zoom);
        var pts = new PointF[]
        {
            new(s.X,           s.Y - r),
            new(s.X + r * 0.6f, s.Y),
            new(s.X,           s.Y + r),
            new(s.X - r * 0.6f, s.Y)
        };
        bool isSel = _selectedDefaultSpawn;
        using var br  = new SolidBrush(Color.LimeGreen);
        using var pen = new Pen(isSel ? Color.Cyan : Color.White, isSel ? 2.5f : 1f);
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.FillPolygon(br, pts);
        g.DrawPolygon(pen, pts);
        g.SmoothingMode = SmoothingMode.None;
        if (_zoom > 0.3f)
        {
            using var f = new Font("Segoe UI", 7f, FontStyle.Bold);
            g.DrawString("SPAWN", f, Brushes.LimeGreen, s.X + r + 2, s.Y - 7);
        }
    }

    private void DrawNamedSpawnPoints(Graphics g)
    {
        if (Map!.SpawnPoints == null || Map.SpawnPoints.Count == 0) return;

        foreach (var kvp in Map.SpawnPoints)
        {
            var sp = kvp.Value;
            var s  = W2S(sp.X, sp.Y);
            float r = Math.Max(5f, 7f * _zoom);
            var pts = new PointF[]
            {
                new(s.X,           s.Y - r),
                new(s.X + r * 0.6f, s.Y),
                new(s.X,           s.Y + r),
                new(s.X - r * 0.6f, s.Y)
            };
            bool isSel = _selectedSpawnKey == kvp.Key;
            using var br  = new SolidBrush(Color.Gold);
            using var pen = new Pen(isSel ? Color.Cyan : Color.White, isSel ? 2.5f : 1f);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.FillPolygon(br, pts);
            g.DrawPolygon(pen, pts);
            g.SmoothingMode = SmoothingMode.None;
            if (_zoom > 0.3f)
            {
                using var f = new Font("Segoe UI", 7f, FontStyle.Bold);
                using var textBrush = new SolidBrush(Color.Gold);
                g.DrawString(kvp.Key, f, textBrush, s.X + r + 2, s.Y - 7);
            }
        }
    }

    private void DrawEnemies(Graphics g)
    {
        if (Map!.Enemies == null || Map.Enemies.Count == 0) return;

        using var patrolPen     = new Pen(Color.FromArgb(150, 200, 100, 60), 1.5f) { DashStyle = DashStyle.Dash };
        using var waypointBrush = new SolidBrush(Color.FromArgb(200, 220, 140, 60));

        for (int i = 0; i < Map.Enemies.Count; i++)
        {
            var en = Map.Enemies[i];

            // Patrol path (dashed line between waypoints)
            var sA = W2S(en.WaypointA.X, en.WaypointA.Y);
            var sB = W2S(en.WaypointB.X, en.WaypointB.Y);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.DrawLine(patrolPen, sA, sB);

            // Waypoint markers
            float wr = Math.Max(3f, 5f * _zoom);
            g.FillEllipse(waypointBrush, sA.X - wr, sA.Y - wr, wr * 2, wr * 2);
            g.FillEllipse(waypointBrush, sB.X - wr, sB.Y - wr, wr * 2, wr * 2);
            g.SmoothingMode = SmoothingMode.None;

            // Enemy body
            var sr = WR2S(en.X, en.Y, en.Width, en.Height);
            using var fill = new SolidBrush(Color.FromArgb(200, 80, 60));
            using var edge = new Pen(Color.FromArgb(120, 0, 0, 0), 1f);
            g.FillRectangle(fill, sr);
            g.DrawRectangle(edge, sr.X, sr.Y, sr.Width, sr.Height);

            // Label
            if (_zoom > 0.3f)
            {
                using var f = new Font("Segoe UI", 7f, FontStyle.Bold);
                g.DrawString($"ENEMY {i}", f, Brushes.OrangeRed, sr.X + 2, sr.Y - 14);
            }
        }
    }

    private void DrawTransitions(Graphics g)
    {
        if (Map!.Transitions == null || Map.Transitions.Count == 0) return;

        using var fillBrush = new SolidBrush(Color.FromArgb(80, 100, 100, 220));
        using var borderPen = new Pen(Color.FromArgb(180, 100, 100, 220), 1.5f) { DashStyle = DashStyle.Dash };

        for (int i = 0; i < Map.Transitions.Count; i++)
        {
            var tr = Map.Transitions[i];
            var sr = WR2S(tr.X, tr.Y, tr.Width, tr.Height);
            g.FillRectangle(fillBrush, sr);
            g.DrawRectangle(borderPen, sr.X, sr.Y, sr.Width, sr.Height);

            if (_zoom > 0.3f)
            {
                using var f = new Font("Segoe UI", 7f, FontStyle.Bold);
                string label = string.IsNullOrEmpty(tr.Name) ? $"TRANSITION {i}" : tr.Name;
                if (!string.IsNullOrEmpty(tr.TargetMap))
                    label += $" → {System.IO.Path.GetFileName(tr.TargetMap)}";
                g.DrawString(label, f, Brushes.CornflowerBlue, sr.X + 2, sr.Y - 14);
            }
        }
    }

    private void DrawPickups(Graphics g)
    {
        if (Map!.AbilityPickups == null || Map.AbilityPickups.Count == 0) return;

        using var fillBrush = new SolidBrush(Color.FromArgb(220, 200, 50));
        using var borderPen = new Pen(Color.FromArgb(255, 180, 160, 20), 2.5f);

        for (int i = 0; i < Map.AbilityPickups.Count; i++)
        {
            var pk = Map.AbilityPickups[i];
            var sr = WR2S(pk.X, pk.Y, pk.Width, pk.Height);
            g.FillRectangle(fillBrush, sr);
            g.DrawRectangle(borderPen, sr.X, sr.Y, sr.Width, sr.Height);

            if (_zoom > 0.3f)
            {
                using var f = new Font("Segoe UI", 7f, FontStyle.Bold);
                string label = string.IsNullOrEmpty(pk.Ability) ? "PICKUP" : pk.Ability;
                g.DrawString(label, f, Brushes.Gold, sr.X + 2, sr.Y - 14);
            }
        }
    }

    // ── Handle geometry ───────────────────────────────────────────────────────
    private Dictionary<ResizeHandle, RectangleF> GetHandleRects(float wx, float wy, float ww, float wh)
    {
        var   s  = WR2S(wx, wy, ww, wh);
        float h  = HandlePx, hh = h / 2f;
        float mx = s.X + s.Width / 2f, my = s.Y + s.Height / 2f;
        float r  = s.Right, b = s.Bottom;

        static RectangleF R(float x, float y, float sz) => new(x, y, sz, sz);
        return new()
        {
            [ResizeHandle.NW] = R(s.X - hh, s.Y - hh, h),
            [ResizeHandle.N]  = R(mx  - hh, s.Y - hh, h),
            [ResizeHandle.NE] = R(r   - hh, s.Y - hh, h),
            [ResizeHandle.E]  = R(r   - hh, my  - hh, h),
            [ResizeHandle.SE] = R(r   - hh, b   - hh, h),
            [ResizeHandle.S]  = R(mx  - hh, b   - hh, h),
            [ResizeHandle.SW] = R(s.X - hh, b   - hh, h),
            [ResizeHandle.W]  = R(s.X - hh, my  - hh, h),
        };
    }

    private ResizeHandle HitHandle(Point screen)
    {
        float wx, wy, ww, wh;
        if (_selected != null)
        {
            wx = _selected.X; wy = _selected.Y; ww = _selected.Width; wh = _selected.Height;
        }
        else if (_selectedEnemy != null)
        {
            wx = _selectedEnemy.X; wy = _selectedEnemy.Y; ww = _selectedEnemy.Width; wh = _selectedEnemy.Height;
        }
        else if (_selectedTransition != null)
        {
            wx = _selectedTransition.X; wy = _selectedTransition.Y; ww = _selectedTransition.Width; wh = _selectedTransition.Height;
        }
        else if (_selectedPickup != null)
        {
            wx = _selectedPickup.X; wy = _selectedPickup.Y; ww = _selectedPickup.Width; wh = _selectedPickup.Height;
        }
        else return ResizeHandle.None;

        foreach (var (handle, rect) in GetHandleRects(wx, wy, ww, wh))
            if (rect.Contains(screen.X, screen.Y)) return handle;
        return ResizeHandle.None;
    }

    private PlatformData? HitPlatform(PointF world)
    {
        for (int i = Map!.Platforms.Count - 1; i >= 0; i--)
        {
            var p = Map.Platforms[i];
            if (world.X >= p.X && world.X <= p.X + p.Width &&
                world.Y >= p.Y && world.Y <= p.Y + p.Height)
                return p;
        }
        return null;
    }

    private EnemyData? HitEnemy(PointF world)
    {
        if (Map!.Enemies == null) return null;
        for (int i = Map.Enemies.Count - 1; i >= 0; i--)
        {
            var en = Map.Enemies[i];
            if (world.X >= en.X && world.X <= en.X + en.Width &&
                world.Y >= en.Y && world.Y <= en.Y + en.Height)
                return en;
        }
        return null;
    }

    private TransitionData? HitTransition(PointF world)
    {
        if (Map!.Transitions == null) return null;
        for (int i = Map.Transitions.Count - 1; i >= 0; i--)
        {
            var tr = Map.Transitions[i];
            if (world.X >= tr.X && world.X <= tr.X + tr.Width &&
                world.Y >= tr.Y && world.Y <= tr.Y + tr.Height)
                return tr;
        }
        return null;
    }

    private AbilityPickupData? HitPickup(PointF world)
    {
        if (Map!.AbilityPickups == null) return null;
        for (int i = Map.AbilityPickups.Count - 1; i >= 0; i--)
        {
            var pk = Map.AbilityPickups[i];
            if (world.X >= pk.X && world.X <= pk.X + pk.Width &&
                world.Y >= pk.Y && world.Y <= pk.Y + pk.Height)
                return pk;
        }
        return null;
    }

    private bool HitDefaultSpawn(PointF world)
    {
        if (Map == null) return false;
        var sp = Map.SpawnPoint;
        float radius = Math.Max(12f, 16f / _zoom);
        float dx = world.X - sp.X;
        float dy = world.Y - sp.Y;
        return dx * dx + dy * dy <= radius * radius;
    }

    private string? HitNamedSpawn(PointF world)
    {
        if (Map?.SpawnPoints == null || Map.SpawnPoints.Count == 0) return null;
        float bestDist = float.MaxValue;
        string? bestKey = null;
        float radius = Math.Max(12f, 16f / _zoom);
        float r2 = radius * radius;
        foreach (var kvp in Map.SpawnPoints)
        {
            float dx = world.X - kvp.Value.X;
            float dy = world.Y - kvp.Value.Y;
            float dist = dx * dx + dy * dy;
            if (dist <= r2 && dist < bestDist)
            {
                bestDist = dist;
                bestKey = kvp.Key;
            }
        }
        return bestKey;
    }

    // ── Mouse ─────────────────────────────────────────────────────────────────
    protected override void OnMouseWheel(MouseEventArgs e)
    {
        var wBefore = S2W(e.Location);
        float factor = e.Delta > 0 ? 1.12f : 1f / 1.12f;
        _zoom = Math.Clamp(_zoom * factor, 0.03f, 12f);
        var wAfter = S2W(e.Location);
        _panX += wBefore.X - wAfter.X;
        _panY += wBefore.Y - wAfter.Y;
        ZoomChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    protected override void OnMouseDown(MouseEventArgs e)
    {
        Focus();
        _lastScreen = e.Location;
        var world = S2W(e.Location);

        if (e.Button == MouseButtons.Middle || e.Button == MouseButtons.Right)
        {
            _isPanning = true;
            Cursor = Cursors.SizeAll;
            return;
        }

        if (e.Button != MouseButtons.Left || Map == null) return;

        if (_tool == EditorTool.Draw)
        {
            _isDrawing      = true;
            _dragStartWorld = new(Snap(world.X), Snap(world.Y));
            _drawRect       = new(_dragStartWorld.X, _dragStartWorld.Y, 0, 0);
        }
        else if (_tool == EditorTool.DrawEnemy)
        {
            var en = new EnemyData
            {
                X = Snap(world.X), Y = Snap(world.Y),
                Width = 40, Height = 40,
                Speed = 100, Damage = 10, Hp = 50,
                WaypointA = new PointData { X = Snap(world.X) - 100, Y = Snap(world.Y) },
                WaypointB = new PointData { X = Snap(world.X) + 100, Y = Snap(world.Y) }
            };
            Map.Enemies.Add(en);
            SelectEnemy(en);
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
        else if (_tool == EditorTool.DrawTransition)
        {
            _isDrawing      = true;
            _dragStartWorld = new(Snap(world.X), Snap(world.Y));
            _drawRect       = new(_dragStartWorld.X, _dragStartWorld.Y, 0, 0);
        }
        else if (_tool == EditorTool.DrawPickup)
        {
            int nextId = (Map.AbilityPickups?.Count ?? 0) + 1;
            var pk = new AbilityPickupData
            {
                Id = $"pickup_{nextId:D2}",
                Ability = "DoubleJump",
                X = Snap(world.X), Y = Snap(world.Y),
                Width = 30, Height = 30
            };
            Map.AbilityPickups.Add(pk);
            SelectPickup(pk);
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
        else if (_tool == EditorTool.DrawSpawnPoint)
        {
            int nextId = (Map.SpawnPoints?.Count ?? 0) + 1;
            string name = $"spawn_{nextId}";
            while (Map.SpawnPoints!.ContainsKey(name))
            {
                nextId++;
                name = $"spawn_{nextId}";
            }
            var pt = new PointData { X = Snap(world.X), Y = Snap(world.Y) };
            Map.SpawnPoints[name] = pt;
            SelectNamedSpawn(name);
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
        else
        {
            var handle = HitHandle(e.Location);
            if (handle != ResizeHandle.None)
            {
                _isDragging     = true;
                _activeHandle   = handle;
                if (_selected != null)
                {
                    _origRect = new(_selected.X, _selected.Y, _selected.Width, _selected.Height);
                }
                else if (_selectedEnemy != null)
                {
                    _origRect       = new(_selectedEnemy.X, _selectedEnemy.Y, _selectedEnemy.Width, _selectedEnemy.Height);
                    _origWaypointA  = new(_selectedEnemy.WaypointA.X, _selectedEnemy.WaypointA.Y);
                    _origWaypointB  = new(_selectedEnemy.WaypointB.X, _selectedEnemy.WaypointB.Y);
                }
                else if (_selectedTransition != null)
                {
                    _origRect = new(_selectedTransition.X, _selectedTransition.Y, _selectedTransition.Width, _selectedTransition.Height);
                }
                else if (_selectedPickup != null)
                {
                    _origRect = new(_selectedPickup.X, _selectedPickup.Y, _selectedPickup.Width, _selectedPickup.Height);
                }
                _dragStartWorld = world;
            }
            else
            {
                // Hit-test spawns first (drawn on top of other objects)
                var hitNamedSpawn = HitNamedSpawn(world);
                if (hitNamedSpawn != null)
                {
                    var sp = Map.SpawnPoints[hitNamedSpawn];
                    SelectNamedSpawn(hitNamedSpawn);
                    _isDragging     = true;
                    _activeHandle   = ResizeHandle.Move;
                    _moveOffX       = world.X - sp.X;
                    _moveOffY       = world.Y - sp.Y;
                    _dragStartWorld = world;
                }
                else if (HitDefaultSpawn(world))
                {
                    var sp = Map.SpawnPoint;
                    SelectDefaultSpawn();
                    _isDragging     = true;
                    _activeHandle   = ResizeHandle.Move;
                    _moveOffX       = world.X - sp.X;
                    _moveOffY       = world.Y - sp.Y;
                    _dragStartWorld = world;
                }
                else
                {
                var hit = HitPlatform(world);
                if (hit != null)
                {
                    SelectPlatform(hit);
                    _isDragging     = true;
                    _activeHandle   = ResizeHandle.Move;
                    _moveOffX       = world.X - hit.X;
                    _moveOffY       = world.Y - hit.Y;
                    _origRect       = new(hit.X, hit.Y, hit.Width, hit.Height);
                    _dragStartWorld = world;
                }
                else
                {
                    var hitEnemy = HitEnemy(world);
                    if (hitEnemy != null)
                    {
                        SelectEnemy(hitEnemy);
                        _isDragging     = true;
                        _activeHandle   = ResizeHandle.Move;
                        _moveOffX       = world.X - hitEnemy.X;
                        _moveOffY       = world.Y - hitEnemy.Y;
                        _origRect       = new(hitEnemy.X, hitEnemy.Y, hitEnemy.Width, hitEnemy.Height);
                        _origWaypointA  = new(hitEnemy.WaypointA.X, hitEnemy.WaypointA.Y);
                        _origWaypointB  = new(hitEnemy.WaypointB.X, hitEnemy.WaypointB.Y);
                        _dragStartWorld = world;
                    }
                    else
                    {
                        var hitTrans = HitTransition(world);
                        if (hitTrans != null)
                        {
                            SelectTransition(hitTrans);
                            _isDragging     = true;
                            _activeHandle   = ResizeHandle.Move;
                            _moveOffX       = world.X - hitTrans.X;
                            _moveOffY       = world.Y - hitTrans.Y;
                            _origRect       = new(hitTrans.X, hitTrans.Y, hitTrans.Width, hitTrans.Height);
                            _dragStartWorld = world;
                        }
                        else
                        {
                            var hitPickup = HitPickup(world);
                            if (hitPickup != null)
                            {
                                SelectPickup(hitPickup);
                                _isDragging     = true;
                                _activeHandle   = ResizeHandle.Move;
                                _moveOffX       = world.X - hitPickup.X;
                                _moveOffY       = world.Y - hitPickup.Y;
                                _origRect       = new(hitPickup.X, hitPickup.Y, hitPickup.Width, hitPickup.Height);
                                _dragStartWorld = world;
                            }
                            else
                            {
                                ClearSelection();
                            }
                        }
                    }
                }
                }
            }
            Invalidate();
        }
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
        var world = S2W(e.Location);

        if (_isPanning)
        {
            _panX -= (e.Location.X - _lastScreen.X) / _zoom;
            _panY -= (e.Location.Y - _lastScreen.Y) / _zoom;
            _lastScreen = e.Location;
            Invalidate();
            return;
        }

        if (_isDrawing)
        {
            _drawRect = new(_dragStartWorld.X, _dragStartWorld.Y,
                Snap(world.X) - _dragStartWorld.X,
                Snap(world.Y) - _dragStartWorld.Y);
            Invalidate();
            return;
        }

        if (_isDragging && (_selected != null || _selectedEnemy != null || _selectedTransition != null || _selectedPickup != null || _selectedDefaultSpawn || _selectedSpawnKey != null))
        {
            if (_selected != null)
            {
                if (_activeHandle == ResizeHandle.Move)
                {
                    _selected.X = Snap(world.X - _moveOffX);
                    _selected.Y = Snap(world.Y - _moveOffY);
                }
                else
                {
                    ApplyResize(world);
                }
            }
            else if (_selectedEnemy != null)
            {
                if (_activeHandle == ResizeHandle.Move)
                {
                    float newX = Snap(world.X - _moveOffX);
                    float newY = Snap(world.Y - _moveOffY);
                    float dx   = newX - _origRect.X;
                    float dy   = newY - _origRect.Y;
                    _selectedEnemy.X = newX;
                    _selectedEnemy.Y = newY;
                    _selectedEnemy.WaypointA.X = _origWaypointA.X + dx;
                    _selectedEnemy.WaypointA.Y = _origWaypointA.Y + dy;
                    _selectedEnemy.WaypointB.X = _origWaypointB.X + dx;
                    _selectedEnemy.WaypointB.Y = _origWaypointB.Y + dy;
                }
                else
                {
                    ApplyResizeEnemy(world);
                }
            }
            else if (_selectedTransition != null)
            {
                if (_activeHandle == ResizeHandle.Move)
                {
                    _selectedTransition.X = Snap(world.X - _moveOffX);
                    _selectedTransition.Y = Snap(world.Y - _moveOffY);
                }
                else
                {
                    ApplyResizeTransition(world);
                }
            }
            else if (_selectedPickup != null)
            {
                if (_activeHandle == ResizeHandle.Move)
                {
                    _selectedPickup.X = Snap(world.X - _moveOffX);
                    _selectedPickup.Y = Snap(world.Y - _moveOffY);
                }
                else
                {
                    ApplyResizePickup(world);
                }
            }
            else if (_selectedDefaultSpawn)
            {
                Map!.SpawnPoint.X = Snap(world.X - _moveOffX);
                Map.SpawnPoint.Y  = Snap(world.Y - _moveOffY);
            }
            else if (_selectedSpawnKey != null && Map!.SpawnPoints.TryGetValue(_selectedSpawnKey, out var sp))
            {
                sp.X = Snap(world.X - _moveOffX);
                sp.Y = Snap(world.Y - _moveOffY);
            }
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
            return;
        }

        UpdateCursor(e.Location, world);
        _lastScreen = e.Location;
    }

    protected override void OnMouseUp(MouseEventArgs e)
    {
        if (_isPanning) { _isPanning = false; UpdateCursorForTool(); return; }

        if (_isDrawing && e.Button == MouseButtons.Left)
        {
            _isDrawing = false;
            var norm   = Normalize(_drawRect);
            if (norm.Width >= 4f && norm.Height >= 4f && Map != null)
            {
                if (_tool == EditorTool.DrawTransition)
                {
                    var tr = new TransitionData
                    {
                        Name = "new_transition",
                        X = norm.X, Y = norm.Y, Width = norm.Width, Height = norm.Height,
                        TargetMap = "", TargetSpawn = "default"
                    };
                    Map.Transitions.Add(tr);
                    SelectTransition(tr);
                }
                else
                {
                    var plat = new PlatformData
                    {
                        X = norm.X, Y = norm.Y, Width = norm.Width, Height = norm.Height,
                        R = 120, G = 80, B = 40
                    };
                    Map.Platforms.Add(plat);
                    SelectPlatform(plat);
                }
                MapChanged?.Invoke(this, EventArgs.Empty);
            }
            Invalidate();
            return;
        }

        if (_isDragging) { _isDragging = false; _activeHandle = ResizeHandle.None; }
    }

    // ── Resize ────────────────────────────────────────────────────────────────
    private (float x, float y, float w, float h) ComputeResize(PointF world)
    {
        var   o  = _origRect;
        float dx = Snap(world.X) - Snap(_dragStartWorld.X);
        float dy = Snap(world.Y) - Snap(_dragStartWorld.Y);
        float x  = o.X, y = o.Y, w = o.Width, h = o.Height;

        switch (_activeHandle)
        {
            case ResizeHandle.N:  y += dy; h -= dy; break;
            case ResizeHandle.S:  h += dy; break;
            case ResizeHandle.W:  x += dx; w -= dx; break;
            case ResizeHandle.E:  w += dx; break;
            case ResizeHandle.NW: x += dx; w -= dx; y += dy; h -= dy; break;
            case ResizeHandle.NE: w += dx;           y += dy; h -= dy; break;
            case ResizeHandle.SW: x += dx; w -= dx;  h += dy; break;
            case ResizeHandle.SE: w += dx;            h += dy; break;
        }

        const float minSize = 5f;
        if (w < minSize) { if (_activeHandle is ResizeHandle.W or ResizeHandle.NW or ResizeHandle.SW) x = o.X + o.Width - minSize; w = minSize; }
        if (h < minSize) { if (_activeHandle is ResizeHandle.N or ResizeHandle.NW or ResizeHandle.NE) y = o.Y + o.Height - minSize; h = minSize; }

        return (x, y, w, h);
    }

    private void ApplyResize(PointF world)
    {
        var (x, y, w, h) = ComputeResize(world);
        _selected!.X = x; _selected.Y = y; _selected.Width = w; _selected.Height = h;
    }

    private void ApplyResizeEnemy(PointF world)
    {
        var (x, y, w, h) = ComputeResize(world);
        _selectedEnemy!.X = x; _selectedEnemy.Y = y; _selectedEnemy.Width = w; _selectedEnemy.Height = h;
    }

    private void ApplyResizeTransition(PointF world)
    {
        var (x, y, w, h) = ComputeResize(world);
        _selectedTransition!.X = x; _selectedTransition.Y = y; _selectedTransition.Width = w; _selectedTransition.Height = h;
    }

    private void ApplyResizePickup(PointF world)
    {
        var (x, y, w, h) = ComputeResize(world);
        _selectedPickup!.X = x; _selectedPickup.Y = y; _selectedPickup.Width = w; _selectedPickup.Height = h;
    }

    // ── Cursor ────────────────────────────────────────────────────────────────
    private void UpdateCursorForTool() => Cursor = (_tool == EditorTool.Draw || _tool == EditorTool.DrawEnemy || _tool == EditorTool.DrawTransition || _tool == EditorTool.DrawPickup || _tool == EditorTool.DrawSpawnPoint) ? Cursors.Cross : Cursors.Default;

    private void UpdateCursor(Point screen, PointF world)
    {
        if (_tool == EditorTool.Draw || _tool == EditorTool.DrawEnemy || _tool == EditorTool.DrawTransition || _tool == EditorTool.DrawPickup || _tool == EditorTool.DrawSpawnPoint) { Cursor = Cursors.Cross; return; }
        var h = HitHandle(screen);
        if (h != ResizeHandle.None) { Cursor = GetResizeCursor(h); return; }
        if (Map != null && HitNamedSpawn(world) != null) { Cursor = Cursors.SizeAll; return; }
        if (Map != null && HitDefaultSpawn(world)) { Cursor = Cursors.SizeAll; return; }
        if (Map != null && HitPlatform(world) != null) { Cursor = Cursors.SizeAll; return; }
        if (Map != null && HitEnemy(world) != null) { Cursor = Cursors.SizeAll; return; }
        if (Map != null && HitTransition(world) != null) { Cursor = Cursors.SizeAll; return; }
        if (Map != null && HitPickup(world) != null) { Cursor = Cursors.SizeAll; return; }
        Cursor = Cursors.Default;
    }

    private static Cursor GetResizeCursor(ResizeHandle h) => h switch
    {
        ResizeHandle.N  or ResizeHandle.S  => Cursors.SizeNS,
        ResizeHandle.E  or ResizeHandle.W  => Cursors.SizeWE,
        ResizeHandle.NW or ResizeHandle.SE => Cursors.SizeNWSE,
        ResizeHandle.NE or ResizeHandle.SW => Cursors.SizeNESW,
        _ => Cursors.Default
    };

    // ── Keyboard ──────────────────────────────────────────────────────────────
    protected override bool IsInputKey(Keys key) =>
        key == Keys.Delete || key == Keys.Escape || base.IsInputKey(key);

    protected override void OnKeyDown(KeyEventArgs e)
    {
        if (e.KeyCode == Keys.Delete) DeleteSelected();
        if (e.KeyCode == Keys.Escape) ClearSelection();
        base.OnKeyDown(e);
    }

    // ── Public operations ─────────────────────────────────────────────────────
    public void DeleteSelected()
    {
        if (Map == null) return;
        if (_selected != null)
        {
            Map.Platforms.Remove(_selected);
            ClearSelection();
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
        else if (_selectedEnemy != null)
        {
            Map.Enemies.Remove(_selectedEnemy);
            ClearSelection();
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
        else if (_selectedTransition != null)
        {
            Map.Transitions.Remove(_selectedTransition);
            ClearSelection();
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
        else if (_selectedPickup != null)
        {
            Map.AbilityPickups.Remove(_selectedPickup);
            ClearSelection();
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
        else if (_selectedSpawnKey != null)
        {
            Map.SpawnPoints.Remove(_selectedSpawnKey);
            ClearSelection();
            MapChanged?.Invoke(this, EventArgs.Empty);
            Invalidate();
        }
    }

    private void SelectPlatform(PlatformData plat)
    {
        _selected           = plat;
        _selectedEnemy      = null;
        _selectedTransition = null;
        _selectedPickup     = null;
        _selectedDefaultSpawn = false;
        _selectedSpawnKey   = null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    private void SelectEnemy(EnemyData enemy)
    {
        _selected           = null;
        _selectedEnemy      = enemy;
        _selectedTransition = null;
        _selectedPickup     = null;
        _selectedDefaultSpawn = false;
        _selectedSpawnKey   = null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    private void SelectTransition(TransitionData transition)
    {
        _selected           = null;
        _selectedEnemy      = null;
        _selectedTransition = transition;
        _selectedPickup     = null;
        _selectedDefaultSpawn = false;
        _selectedSpawnKey   = null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    private void SelectPickup(AbilityPickupData pickup)
    {
        _selected           = null;
        _selectedEnemy      = null;
        _selectedTransition = null;
        _selectedPickup     = pickup;
        _selectedDefaultSpawn = false;
        _selectedSpawnKey   = null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    private void SelectDefaultSpawn()
    {
        _selected           = null;
        _selectedEnemy      = null;
        _selectedTransition = null;
        _selectedPickup     = null;
        _selectedDefaultSpawn = true;
        _selectedSpawnKey   = null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    private void SelectNamedSpawn(string key)
    {
        _selected           = null;
        _selectedEnemy      = null;
        _selectedTransition = null;
        _selectedPickup     = null;
        _selectedDefaultSpawn = false;
        _selectedSpawnKey   = key;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    public void SelectSpawnByKey(string? key)
    {
        if (key == null) ClearSelection();
        else if (key == "") SelectDefaultSpawn();
        else SelectNamedSpawn(key);
    }

    private void ClearSelection()
    {
        _selected           = null;
        _selectedEnemy      = null;
        _selectedTransition = null;
        _selectedPickup     = null;
        _selectedDefaultSpawn = false;
        _selectedSpawnKey   = null;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    // ── Utility ───────────────────────────────────────────────────────────────
    private static RectangleF Normalize(RectangleF r) => new(
        r.Width  < 0 ? r.X + r.Width  : r.X,
        r.Height < 0 ? r.Y + r.Height : r.Y,
        MathF.Abs(r.Width), MathF.Abs(r.Height));
}
