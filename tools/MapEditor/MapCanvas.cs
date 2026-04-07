using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace MapEditor;

public enum EditorTool { Select, Draw }
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
        Map       = map;
        _selected = null;
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
        if (_isDrawing)       DrawGhost(g);
        if (_selected != null) { DrawOutline(g, _selected); DrawHandles(g, _selected); }
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

    private void DrawOutline(Graphics g, PlatformData p)
    {
        var sr = WR2S(p.X, p.Y, p.Width, p.Height);
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

    private void DrawHandles(Graphics g, PlatformData p)
    {
        foreach (var (_, rect) in GetHandleRects(p))
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
        using var br  = new SolidBrush(Color.LimeGreen);
        using var pen = new Pen(Color.White, 1f);
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
            using var br  = new SolidBrush(Color.Gold);
            using var pen = new Pen(Color.White, 1f);
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

    // ── Handle geometry ───────────────────────────────────────────────────────
    private Dictionary<ResizeHandle, RectangleF> GetHandleRects(PlatformData p)
    {
        var   s  = WR2S(p.X, p.Y, p.Width, p.Height);
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
        if (_selected == null) return ResizeHandle.None;
        foreach (var (handle, rect) in GetHandleRects(_selected))
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
        else
        {
            var handle = HitHandle(e.Location);
            if (handle != ResizeHandle.None)
            {
                _isDragging     = true;
                _activeHandle   = handle;
                _origRect       = new(_selected!.X, _selected.Y, _selected.Width, _selected.Height);
                _dragStartWorld = world;
            }
            else
            {
                var hit = HitPlatform(world);
                if (hit != null)
                {
                    SetSelected(hit);
                    _isDragging     = true;
                    _activeHandle   = ResizeHandle.Move;
                    _moveOffX       = world.X - hit.X;
                    _moveOffY       = world.Y - hit.Y;
                    _origRect       = new(hit.X, hit.Y, hit.Width, hit.Height);
                    _dragStartWorld = world;
                }
                else
                {
                    SetSelected(null);
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

        if (_isDragging && _selected != null)
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
                var plat = new PlatformData
                {
                    X = norm.X, Y = norm.Y, Width = norm.Width, Height = norm.Height,
                    R = 120, G = 80, B = 40
                };
                Map.Platforms.Add(plat);
                SetSelected(plat);
                MapChanged?.Invoke(this, EventArgs.Empty);
            }
            Invalidate();
            return;
        }

        if (_isDragging) { _isDragging = false; _activeHandle = ResizeHandle.None; }
    }

    // ── Resize ────────────────────────────────────────────────────────────────
    private void ApplyResize(PointF world)
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

        _selected!.X = x; _selected.Y = y; _selected.Width = w; _selected.Height = h;
    }

    // ── Cursor ────────────────────────────────────────────────────────────────
    private void UpdateCursorForTool() => Cursor = _tool == EditorTool.Draw ? Cursors.Cross : Cursors.Default;

    private void UpdateCursor(Point screen, PointF world)
    {
        if (_tool == EditorTool.Draw) { Cursor = Cursors.Cross; return; }
        var h = HitHandle(screen);
        if (h != ResizeHandle.None) { Cursor = GetResizeCursor(h); return; }
        if (Map != null && HitPlatform(world) != null) { Cursor = Cursors.SizeAll; return; }
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
        if (e.KeyCode == Keys.Escape) SetSelected(null);
        base.OnKeyDown(e);
    }

    // ── Public operations ─────────────────────────────────────────────────────
    public void DeleteSelected()
    {
        if (_selected == null || Map == null) return;
        Map.Platforms.Remove(_selected);
        SetSelected(null);
        MapChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    private void SetSelected(PlatformData? plat)
    {
        _selected = plat;
        SelectionChanged?.Invoke(this, EventArgs.Empty);
        Invalidate();
    }

    // ── Utility ───────────────────────────────────────────────────────────────
    private static RectangleF Normalize(RectangleF r) => new(
        r.Width  < 0 ? r.X + r.Width  : r.X,
        r.Height < 0 ? r.Y + r.Height : r.Y,
        MathF.Abs(r.Width), MathF.Abs(r.Height));
}
