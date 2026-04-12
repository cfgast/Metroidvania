using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace MapEditor;

public class MapData
{
    [JsonPropertyName("name")]
    public string Name { get; set; } = "New Map";

    [JsonPropertyName("bounds")]
    public BoundsData Bounds { get; set; } = new();

    [JsonPropertyName("spawnPoint")]
    public PointData SpawnPoint { get; set; } = new();

    [JsonPropertyName("spawnPoints")]
    public Dictionary<string, PointData> SpawnPoints { get; set; } = new();

    [JsonPropertyName("platforms")]
    public List<PlatformData> Platforms { get; set; } = new();

    [JsonPropertyName("enemies")]
    public List<EnemyData> Enemies { get; set; } = new();

    [JsonPropertyName("transitions")]
    public List<TransitionData> Transitions { get; set; } = new();

    [JsonPropertyName("abilityPickups")]
    public List<AbilityPickupData> AbilityPickups { get; set; } = new();
}

public class BoundsData
{
    [JsonPropertyName("x")]      public float X      { get; set; } = -200f;
    [JsonPropertyName("y")]      public float Y      { get; set; } = -500f;
    [JsonPropertyName("width")]  public float Width  { get; set; } = 3600f;
    [JsonPropertyName("height")] public float Height { get; set; } = 1200f;
}

public class PointData
{
    [JsonPropertyName("x")] public float X { get; set; } = 150f;
    [JsonPropertyName("y")] public float Y { get; set; } = 475f;
}

public class PlatformData
{
    [JsonPropertyName("x")]      public float X      { get; set; }
    [JsonPropertyName("y")]      public float Y      { get; set; }
    [JsonPropertyName("width")]  public float Width  { get; set; } = 100f;
    [JsonPropertyName("height")] public float Height { get; set; } = 20f;
    [JsonPropertyName("r")]      public int   R      { get; set; } = 100;
    [JsonPropertyName("g")]      public int   G      { get; set; } = 100;
    [JsonPropertyName("b")]      public int   B      { get; set; } = 100;
}

public class EnemyData
{
    [JsonPropertyName("x")]         public float X         { get; set; }
    [JsonPropertyName("y")]         public float Y         { get; set; }
    [JsonPropertyName("waypointA")] public PointData WaypointA { get; set; } = new();
    [JsonPropertyName("waypointB")] public PointData WaypointB { get; set; } = new();
    [JsonPropertyName("speed")]     public float Speed     { get; set; } = 100f;
    [JsonPropertyName("damage")]    public float Damage    { get; set; } = 10f;
    [JsonPropertyName("hp")]        public float Hp        { get; set; } = 50f;
    [JsonPropertyName("width")]     public float Width     { get; set; } = 40f;
    [JsonPropertyName("height")]    public float Height    { get; set; } = 40f;
}

public class TransitionData
{
    [JsonPropertyName("name")]        public string Name        { get; set; } = "";
    [JsonPropertyName("x")]           public float  X           { get; set; }
    [JsonPropertyName("y")]           public float  Y           { get; set; }
    [JsonPropertyName("width")]       public float  Width       { get; set; }
    [JsonPropertyName("height")]      public float  Height      { get; set; }
    [JsonPropertyName("targetMap")]   public string TargetMap   { get; set; } = "";
    [JsonPropertyName("targetSpawn")] public string TargetSpawn { get; set; } = "default";
}

public class AbilityPickupData
{
    [JsonPropertyName("id")]      public string Id      { get; set; } = "";
    [JsonPropertyName("ability")] public string Ability  { get; set; } = "DoubleJump";
    [JsonPropertyName("x")]       public float  X       { get; set; }
    [JsonPropertyName("y")]       public float  Y       { get; set; }
    [JsonPropertyName("width")]   public float  Width   { get; set; } = 30f;
    [JsonPropertyName("height")]  public float  Height  { get; set; } = 30f;
}

/// <summary>
/// Wraps a single map in the multi-map document model, tracking its global
/// world-space position, file path, and dirty state.
/// </summary>
public class EditorMap
{
    public MapData Map      { get; set; } = null!;
    public float   WorldX   { get; set; }
    public float   WorldY   { get; set; }
    public string  FilePath { get; set; } = "";
    public bool    IsDirty  { get; set; }
}

/// <summary>
/// An entry in a world file referencing a single map and its global position.
/// </summary>
public class WorldMapEntry
{
    [JsonPropertyName("path")] public string Path { get; set; } = "";
    [JsonPropertyName("x")]    public float  X    { get; set; }
    [JsonPropertyName("y")]    public float  Y    { get; set; }
}

/// <summary>
/// Top-level world file model that aggregates multiple maps with world-space offsets.
/// World files use the .world.json extension.
/// </summary>
public class WorldData
{
    [JsonPropertyName("maps")] public List<WorldMapEntry> Maps { get; set; } = new();
}
