using System;
using System.Collections.Generic;
using System.IO;

namespace MapEditor;

/// <summary>
/// Detects edge adjacency between maps and auto-generates transition zones
/// and spawn points for each shared edge.
/// </summary>
public static class TransitionGenerator
{
    private const float TransitionDepth = 50f;
    private const float SpawnOffset = 60f;
    private const string AutoSpawnPrefix = "from_";

    /// <summary>
    /// Regenerates all transitions and auto-spawn points for the given maps.
    /// Clears existing transitions and auto-generated spawn points first,
    /// then detects adjacency between all pairs and generates new ones.
    /// </summary>
    public static void RegenerateTransitions(List<EditorMap> allMaps)
    {
        // Clear all existing transitions and auto-generated spawn points
        foreach (var em in allMaps)
        {
            em.Map.Transitions.Clear();

            var keysToRemove = new List<string>();
            foreach (var key in em.Map.SpawnPoints.Keys)
            {
                if (key.StartsWith(AutoSpawnPrefix, StringComparison.OrdinalIgnoreCase))
                    keysToRemove.Add(key);
            }
            foreach (var key in keysToRemove)
                em.Map.SpawnPoints.Remove(key);
        }

        // Check all pairs of maps for adjacency
        for (int i = 0; i < allMaps.Count; i++)
        {
            for (int j = i + 1; j < allMaps.Count; j++)
            {
                DetectAndGenerate(allMaps[i], allMaps[j]);
            }
        }
    }

    /// <summary>
    /// Returns the map file name without extension, used for naming
    /// transitions and spawn points.
    /// </summary>
    private static string MapKey(EditorMap em)
    {
        if (!string.IsNullOrEmpty(em.FilePath))
            return Path.GetFileNameWithoutExtension(em.FilePath);
        return em.Map.Name ?? "unknown";
    }

    /// <summary>
    /// Checks all four edge orientations between two maps and generates
    /// transition data for any shared edges found.
    /// </summary>
    private static void DetectAndGenerate(EditorMap mapA, EditorMap mapB)
    {
        var bA = mapA.Map.Bounds;
        var bB = mapB.Map.Bounds;

        float aLeft   = mapA.WorldX + bA.X;
        float aRight  = mapA.WorldX + bA.X + bA.Width;
        float aTop    = mapA.WorldY + bA.Y;
        float aBottom = mapA.WorldY + bA.Y + bA.Height;

        float bLeft   = mapB.WorldX + bB.X;
        float bRight  = mapB.WorldX + bB.X + bB.Width;
        float bTop    = mapB.WorldY + bB.Y;
        float bBottom = mapB.WorldY + bB.Y + bB.Height;

        // A-right ↔ B-left
        if (MathF.Abs(aRight - bLeft) < 0.5f)
        {
            float overlapTop    = MathF.Max(aTop, bTop);
            float overlapBottom = MathF.Min(aBottom, bBottom);
            if (overlapBottom > overlapTop)
                GenerateHorizontal(mapA, mapB, aRight, overlapTop, overlapBottom);
        }

        // A-left ↔ B-right
        if (MathF.Abs(aLeft - bRight) < 0.5f)
        {
            float overlapTop    = MathF.Max(aTop, bTop);
            float overlapBottom = MathF.Min(aBottom, bBottom);
            if (overlapBottom > overlapTop)
                GenerateHorizontal(mapB, mapA, aLeft, overlapTop, overlapBottom);
        }

        // A-bottom ↔ B-top
        if (MathF.Abs(aBottom - bTop) < 0.5f)
        {
            float overlapLeft  = MathF.Max(aLeft, bLeft);
            float overlapRight = MathF.Min(aRight, bRight);
            if (overlapRight > overlapLeft)
                GenerateVertical(mapA, mapB, aBottom, overlapLeft, overlapRight);
        }

        // A-top ↔ B-bottom
        if (MathF.Abs(aTop - bBottom) < 0.5f)
        {
            float overlapLeft  = MathF.Max(aLeft, bLeft);
            float overlapRight = MathF.Min(aRight, bRight);
            if (overlapRight > overlapLeft)
                GenerateVertical(mapB, mapA, aTop, overlapLeft, overlapRight);
        }
    }

    /// <summary>
    /// Generates transition zones and spawn points for a horizontal adjacency
    /// where leftMap's right edge meets rightMap's left edge.
    /// </summary>
    private static void GenerateHorizontal(
        EditorMap leftMap, EditorMap rightMap,
        float worldEdgeX, float overlapTop, float overlapBottom)
    {
        float overlapHeight = overlapBottom - overlapTop;
        string leftKey  = MapKey(leftMap);
        string rightKey = MapKey(rightMap);

        // Transition zone in leftMap along its right edge (extends inward)
        leftMap.Map.Transitions.Add(new TransitionData
        {
            Name        = $"to_{rightKey}",
            X           = worldEdgeX - TransitionDepth - leftMap.WorldX,
            Y           = overlapTop - leftMap.WorldY,
            Width       = TransitionDepth,
            Height      = overlapHeight,
            TargetMap   = rightMap.FilePath,
            TargetSpawn = $"from_{leftKey}"
        });

        // Transition zone in rightMap along its left edge (extends inward)
        rightMap.Map.Transitions.Add(new TransitionData
        {
            Name        = $"to_{leftKey}",
            X           = worldEdgeX - rightMap.WorldX,
            Y           = overlapTop - rightMap.WorldY,
            Width       = TransitionDepth,
            Height      = overlapHeight,
            TargetMap   = leftMap.FilePath,
            TargetSpawn = $"from_{rightKey}"
        });

        // Spawn point in leftMap: 60 units inward from the right edge
        float midY = overlapTop + overlapHeight / 2f;
        leftMap.Map.SpawnPoints[$"from_{rightKey}"] = new PointData
        {
            X = worldEdgeX - SpawnOffset - leftMap.WorldX,
            Y = midY - leftMap.WorldY
        };

        // Spawn point in rightMap: 60 units inward from the left edge
        rightMap.Map.SpawnPoints[$"from_{leftKey}"] = new PointData
        {
            X = worldEdgeX + SpawnOffset - rightMap.WorldX,
            Y = midY - rightMap.WorldY
        };
    }

    /// <summary>
    /// Generates transition zones and spawn points for a vertical adjacency
    /// where topMap's bottom edge meets bottomMap's top edge.
    /// </summary>
    private static void GenerateVertical(
        EditorMap topMap, EditorMap bottomMap,
        float worldEdgeY, float overlapLeft, float overlapRight)
    {
        float overlapWidth = overlapRight - overlapLeft;
        string topKey    = MapKey(topMap);
        string bottomKey = MapKey(bottomMap);

        // Transition zone in topMap along its bottom edge (extends inward/upward)
        topMap.Map.Transitions.Add(new TransitionData
        {
            Name        = $"to_{bottomKey}",
            X           = overlapLeft - topMap.WorldX,
            Y           = worldEdgeY - TransitionDepth - topMap.WorldY,
            Width       = overlapWidth,
            Height      = TransitionDepth,
            TargetMap   = bottomMap.FilePath,
            TargetSpawn = $"from_{topKey}"
        });

        // Transition zone in bottomMap along its top edge (extends inward/downward)
        bottomMap.Map.Transitions.Add(new TransitionData
        {
            Name        = $"to_{topKey}",
            X           = overlapLeft - bottomMap.WorldX,
            Y           = worldEdgeY - bottomMap.WorldY,
            Width       = overlapWidth,
            Height      = TransitionDepth,
            TargetMap   = topMap.FilePath,
            TargetSpawn = $"from_{bottomKey}"
        });

        // Spawn point in topMap: 60 units inward from the bottom edge
        float midX = overlapLeft + overlapWidth / 2f;
        topMap.Map.SpawnPoints[$"from_{bottomKey}"] = new PointData
        {
            X = midX - topMap.WorldX,
            Y = worldEdgeY - SpawnOffset - topMap.WorldY
        };

        // Spawn point in bottomMap: 60 units inward from the top edge
        bottomMap.Map.SpawnPoints[$"from_{topKey}"] = new PointData
        {
            X = midX - bottomMap.WorldX,
            Y = worldEdgeY + SpawnOffset - bottomMap.WorldY
        };
    }
}
