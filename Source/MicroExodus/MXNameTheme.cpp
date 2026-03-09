// MXNameTheme.cpp — Identity Module: Per-Robot Themed Name Packs
// Created: 2026-03-09

#include "MXNameTheme.h"
#include "Engine/DataTable.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMXNameThemeManager::UMXNameThemeManager()
{
}

// ---------------------------------------------------------------------------
// Initialize
// ---------------------------------------------------------------------------

void UMXNameThemeManager::Initialize()
{
    ThemeRegistry.Empty();

    RegisterRobotTheme();
    RegisterWizardTheme();
    RegisterPirateTheme();
    RegisterSamuraiTheme();
    RegisterSciFiTheme();
    RegisterMythicTheme();

    UE_LOG(LogTemp, Log, TEXT("MXNameThemeManager: Initialized %d themes."), ThemeRegistry.Num());
}

// ---------------------------------------------------------------------------
// RegisterCustomTheme
// ---------------------------------------------------------------------------

void UMXNameThemeManager::RegisterCustomTheme(const FString& ThemeName,
                                                UDataTable* FirstNameTable,
                                                UDataTable* SurnameTable,
                                                UDataTable* TitleAliasTable,
                                                const FString& InTitlePrefix,
                                                const FString& InDescription,
                                                const FString& InUnlockCondition)
{
    FMXNameThemeData Theme;
    Theme.ThemeName = ThemeName;
    Theme.Description = InDescription;
    Theme.UnlockCondition = InUnlockCondition;
    Theme.TitlePrefix = InTitlePrefix;
    Theme.bUnlocked = InUnlockCondition.IsEmpty();

    if (FirstNameTable)
    {
        TArray<FMXThemeFirstNameRow*> Rows;
        FirstNameTable->GetAllRows<FMXThemeFirstNameRow>(TEXT("NameTheme"), Rows);
        for (const auto* Row : Rows)
        {
            if (Row && !Row->Name.IsEmpty()) Theme.FirstNames.Add(Row->Name);
        }
    }

    if (SurnameTable)
    {
        TArray<FMXThemeSurnameRow*> Rows;
        SurnameTable->GetAllRows<FMXThemeSurnameRow>(TEXT("NameTheme"), Rows);
        for (const auto* Row : Rows)
        {
            if (!Row || Row->Surname.IsEmpty()) continue;
            switch (Row->Role)
            {
                case ERobotRole::Scout:    Theme.ScoutSurnames.Add(Row->Surname); break;
                case ERobotRole::Guardian: Theme.GuardianSurnames.Add(Row->Surname); break;
                case ERobotRole::Engineer: Theme.EngineerSurnames.Add(Row->Surname); break;
                default:                   Theme.UniversalSurnames.Add(Row->Surname); break;
            }
        }
    }

    if (TitleAliasTable)
    {
        TArray<FMXThemeTitleAliasRow*> Rows;
        TitleAliasTable->GetAllRows<FMXThemeTitleAliasRow>(TEXT("NameTheme"), Rows);
        for (const auto* Row : Rows)
        {
            if (Row && !Row->OriginalTitle.IsEmpty())
            {
                Theme.TitleAliases.Add(Row->OriginalTitle, Row->ThemedTitle);
            }
        }
    }

    ThemeRegistry.Add(ENameTheme::Custom, Theme);

    UE_LOG(LogTemp, Log, TEXT("MXNameThemeManager: Registered custom theme '%s' (%d names, %d surnames)."),
           *ThemeName, Theme.FirstNames.Num(),
           Theme.UniversalSurnames.Num() + Theme.ScoutSurnames.Num() +
           Theme.GuardianSurnames.Num() + Theme.EngineerSurnames.Num());
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

const FMXNameThemeData& UMXNameThemeManager::GetThemeData(ENameTheme Theme) const
{
    const FMXNameThemeData* Found = ThemeRegistry.Find(Theme);
    return Found ? *Found : EmptyTheme;
}

const TArray<FString>& UMXNameThemeManager::GetFirstNames(ENameTheme Theme) const
{
    const FMXNameThemeData* Found = ThemeRegistry.Find(Theme);
    return Found ? Found->FirstNames : EmptyArray;
}

TArray<FString> UMXNameThemeManager::GetSurnames(ENameTheme Theme, ERobotRole Role) const
{
    const FMXNameThemeData* Found = ThemeRegistry.Find(Theme);
    if (!Found) return TArray<FString>();

    TArray<FString> Combined;

    // Role-specific first.
    switch (Role)
    {
        case ERobotRole::Scout:    Combined.Append(Found->ScoutSurnames); break;
        case ERobotRole::Guardian: Combined.Append(Found->GuardianSurnames); break;
        case ERobotRole::Engineer: Combined.Append(Found->EngineerSurnames); break;
        default: break;
    }

    // Universal fallback.
    Combined.Append(Found->UniversalSurnames);
    return Combined;
}

FString UMXNameThemeManager::GetThemedTitle(ENameTheme Theme, const FString& CanonicalTitle) const
{
    const FMXNameThemeData* Found = ThemeRegistry.Find(Theme);
    if (!Found) return CanonicalTitle;

    const FString* Alias = Found->TitleAliases.Find(CanonicalTitle);
    return Alias ? *Alias : CanonicalTitle;
}

FString UMXNameThemeManager::GetTitlePrefix(ENameTheme Theme) const
{
    const FMXNameThemeData* Found = ThemeRegistry.Find(Theme);
    return Found ? Found->TitlePrefix : TEXT("The");
}

// ---------------------------------------------------------------------------
// Registry Queries
// ---------------------------------------------------------------------------

TArray<ENameTheme> UMXNameThemeManager::GetAllThemes() const
{
    TArray<ENameTheme> Result;
    ThemeRegistry.GetKeys(Result);
    return Result;
}

TArray<ENameTheme> UMXNameThemeManager::GetUnlockedThemes() const
{
    TArray<ENameTheme> Result;
    for (const auto& Pair : ThemeRegistry)
    {
        if (Pair.Value.bUnlocked) Result.Add(Pair.Key);
    }
    return Result;
}

bool UMXNameThemeManager::IsThemeUnlocked(ENameTheme Theme) const
{
    const FMXNameThemeData* Found = ThemeRegistry.Find(Theme);
    return Found && Found->bUnlocked;
}

void UMXNameThemeManager::UnlockTheme(ENameTheme Theme)
{
    FMXNameThemeData* Found = ThemeRegistry.Find(Theme);
    if (Found)
    {
        Found->bUnlocked = true;
        UE_LOG(LogTemp, Log, TEXT("MXNameThemeManager: Unlocked theme '%s'."), *Found->ThemeName);
    }
}

// ===========================================================================
// BUILT-IN THEMES
// ===========================================================================

// ---------------------------------------------------------------------------
// Robot (Default) — always unlocked
// ---------------------------------------------------------------------------

void UMXNameThemeManager::RegisterRobotTheme()
{
    FMXNameThemeData T;
    T.ThemeName = TEXT("Robot");
    T.Description = TEXT("Classic robot names. Bolts, sprockets, and industrial charm.");
    T.bUnlocked = true;
    T.TitlePrefix = TEXT("The");

    // First names — the existing DT_Names pool (subset, full pool loaded from DataTable at runtime).
    T.FirstNames = {
        TEXT("Bolt"),     TEXT("Sprocket"), TEXT("Flicker"),  TEXT("Noodle"),   TEXT("Pip"),
        TEXT("Wobble"),   TEXT("Tink"),     TEXT("Doodle"),   TEXT("Snoot"),    TEXT("Beans"),
        TEXT("Cog"),      TEXT("Rivet"),    TEXT("Socket"),   TEXT("Latch"),    TEXT("Toggle"),
        TEXT("Buzzer"),   TEXT("Clank"),    TEXT("Whirr"),    TEXT("Fizz"),     TEXT("Dash"),
        TEXT("Moss"),     TEXT("Dewdrop"), TEXT("Clover"),   TEXT("Fuse"),     TEXT("Gadget"),
        TEXT("Ratchet"),  TEXT("Diode"),    TEXT("Pixel"),    TEXT("Glitch"),   TEXT("Servo"),
    };

    T.UniversalSurnames = {
        TEXT("Prime"),  TEXT("Nova"),   TEXT("Apex"),   TEXT("Echo"),   TEXT("Hex"),
        TEXT("Flux"),   TEXT("Drift"),  TEXT("Shard"),  TEXT("Pulse"),  TEXT("Cipher"),
    };
    T.ScoutSurnames = {
        TEXT("Swift"),    TEXT("Dash"),     TEXT("Flicker"),  TEXT("Phantom"),
        TEXT("Whisper"),  TEXT("Dart"),     TEXT("Shadow"),   TEXT("Blitz"),
    };
    T.GuardianSurnames = {
        TEXT("Bastion"),  TEXT("Bulwark"),  TEXT("Rampart"),  TEXT("Anvil"),
        TEXT("Fortress"), TEXT("Aegis"),    TEXT("Sentinel"), TEXT("Shield"),
    };
    T.EngineerSurnames = {
        TEXT("Sprocket"), TEXT("Cog"),      TEXT("Rivet"),    TEXT("Piston"),
        TEXT("Socket"),   TEXT("Gear"),     TEXT("Wrench"),   TEXT("Lathe"),
    };

    // No aliases — Robot theme uses canonical titles.

    ThemeRegistry.Add(ENameTheme::Robot, T);
}

// ---------------------------------------------------------------------------
// Wizard
// ---------------------------------------------------------------------------

void UMXNameThemeManager::RegisterWizardTheme()
{
    FMXNameThemeData T;
    T.ThemeName = TEXT("Wizard");
    T.Description = TEXT("Arcane names from the tower. Spells, staffs, and mystical nonsense.");
    T.UnlockCondition = TEXT("Have a robot reach Level 50.");
    T.bUnlocked = false;
    T.TitlePrefix = TEXT("The Great");

    T.FirstNames = {
        TEXT("Ember"),    TEXT("Rune"),     TEXT("Glimmer"),  TEXT("Wisp"),     TEXT("Flint"),
        TEXT("Thistle"),  TEXT("Bramble"),  TEXT("Ash"),      TEXT("Cobalt"),   TEXT("Dusk"),
        TEXT("Fable"),    TEXT("Grimble"),  TEXT("Hazel"),    TEXT("Ink"),      TEXT("Jinx"),
        TEXT("Kindle"),   TEXT("Lumen"),    TEXT("Murk"),     TEXT("Nettle"),   TEXT("Omen"),
        TEXT("Quill"),    TEXT("Sigil"),    TEXT("Thorn"),    TEXT("Vesper"),   TEXT("Wren"),
        TEXT("Zephyr"),   TEXT("Arcana"),   TEXT("Brimble"),  TEXT("Charm"),    TEXT("Dewmist"),
    };

    T.UniversalSurnames = {
        TEXT("Hexfire"),    TEXT("Spellwick"),  TEXT("Moonvane"),   TEXT("Starcap"),
        TEXT("Dustweave"),  TEXT("Glowspark"),  TEXT("Ashmantle"),  TEXT("Fogwhisper"),
        TEXT("Nighthollow"),TEXT("Runemark"),
    };
    T.ScoutSurnames = {
        TEXT("Mistwalker"), TEXT("Shadowveil"), TEXT("Windseeker"), TEXT("Glintstep"),
        TEXT("Faefoot"),    TEXT("Driftshade"), TEXT("Flickercloak"),TEXT("Voidskim"),
    };
    T.GuardianSurnames = {
        TEXT("Wardstone"),  TEXT("Shieldcrest"),TEXT("Ironoak"),    TEXT("Grimwall"),
        TEXT("Barkshield"), TEXT("Stonevow"),   TEXT("Oathbound"),  TEXT("Thornguard"),
    };
    T.EngineerSurnames = {
        TEXT("Gearwand"),   TEXT("Runecog"),    TEXT("Sparkscribe"),TEXT("Coilmancer"),
        TEXT("Boltweaver"), TEXT("Clocksage"),  TEXT("Forgewhisper"),TEXT("Steamrune"),
    };

    T.TitleAliases = {
        {TEXT("Scorched"),       TEXT("Ember-Touched")},
        {TEXT("Firewalker"),     TEXT("Pyromancer")},
        {TEXT("Frostbitten"),    TEXT("Cryomancer")},
        {TEXT("Grounded"),       TEXT("Stormbound")},
        {TEXT("Lucky"),          TEXT("Charmed")},
        {TEXT("Untouchable"),    TEXT("Warded")},
        {TEXT("Veteran"),        TEXT("Sage")},
        {TEXT("Legend"),         TEXT("Archmage")},
        {TEXT("Immortal"),       TEXT("Undying")},
        {TEXT("Haunted"),        TEXT("Spiritseer")},
        {TEXT("Witness"),        TEXT("Oracle")},
        {TEXT("On A Roll"),      TEXT("Enchanted")},
        {TEXT("Unstoppable"),    TEXT("Ascendant")},
        {TEXT("Scarred"),        TEXT("Battle-Hexed")},
    };

    ThemeRegistry.Add(ENameTheme::Wizard, T);
}

// ---------------------------------------------------------------------------
// Pirate
// ---------------------------------------------------------------------------

void UMXNameThemeManager::RegisterPirateTheme()
{
    FMXNameThemeData T;
    T.ThemeName = TEXT("Pirate");
    T.Description = TEXT("Salty sea dogs. Barnacles, grog, and questionable honour.");
    T.UnlockCondition = TEXT("Complete 10 runs.");
    T.bUnlocked = false;
    T.TitlePrefix = TEXT("Captain");

    T.FirstNames = {
        TEXT("Barnacle"),   TEXT("Scurvy"),    TEXT("Plank"),     TEXT("Grog"),      TEXT("Anchor"),
        TEXT("Bilge"),      TEXT("Cutlass"),   TEXT("Flotsam"),   TEXT("Keel"),      TEXT("Maroon"),
        TEXT("Portside"),   TEXT("Rigger"),    TEXT("Salty"),     TEXT("Swab"),      TEXT("Crow"),
        TEXT("Mast"),       TEXT("Powder"),    TEXT("Reef"),      TEXT("Starboard"), TEXT("Stern"),
        TEXT("Jetsam"),     TEXT("Bowline"),   TEXT("Brine"),     TEXT("Knot"),      TEXT("Dagger"),
        TEXT("Galleon"),    TEXT("Hook"),      TEXT("Rum"),       TEXT("Tide"),      TEXT("Compass"),
    };

    T.UniversalSurnames = {
        TEXT("Planksworth"),  TEXT("Saltbeard"),   TEXT("Tideborn"),    TEXT("Deckswab"),
        TEXT("Rumsworth"),    TEXT("Keelhaul"),    TEXT("Blackwater"),  TEXT("Driftwood"),
        TEXT("Stormchaser"),  TEXT("Seaspray"),
    };
    T.ScoutSurnames = {
        TEXT("Crowsnest"),  TEXT("Spyglass"),   TEXT("Windchaser"), TEXT("Swiftcurrent"),
        TEXT("Horizonseer"),TEXT("Quicktide"),  TEXT("Fogrunner"),  TEXT("Riptide"),
    };
    T.GuardianSurnames = {
        TEXT("Ironhull"),   TEXT("Cannonwall"), TEXT("Anchorfast"), TEXT("Broadsider"),
        TEXT("Bulkhead"),   TEXT("Rambeard"),   TEXT("Steelbow"),   TEXT("Chainmast"),
    };
    T.EngineerSurnames = {
        TEXT("Cogswain"),   TEXT("Boltrigger"), TEXT("Gearport"),   TEXT("Pistonmate"),
        TEXT("Boilerdeck"), TEXT("Wrenchport"), TEXT("Steamhook"),  TEXT("Cranksail"),
    };

    T.TitleAliases = {
        {TEXT("Scorched"),       TEXT("Powder Keg")},
        {TEXT("Firewalker"),     TEXT("Cannoneer")},
        {TEXT("Frostbitten"),    TEXT("Frostbitten")},    // Keeps original — cold pirates are just cold.
        {TEXT("Grounded"),       TEXT("Landlocked")},
        {TEXT("Lucky"),          TEXT("Lucky")},           // Pirates keep "Lucky".
        {TEXT("Untouchable"),    TEXT("Uncatchable")},
        {TEXT("Veteran"),        TEXT("Old Salt")},
        {TEXT("Legend"),         TEXT("Dread Pirate")},
        {TEXT("Immortal"),       TEXT("Deathless")},
        {TEXT("Haunted"),        TEXT("Cursed")},
        {TEXT("Witness"),        TEXT("Lookout")},
        {TEXT("On A Roll"),      TEXT("On A Plunder")},
        {TEXT("Unstoppable"),    TEXT("Scourge")},
        {TEXT("Scarred"),        TEXT("Battleworn")},
    };

    ThemeRegistry.Add(ENameTheme::Pirate, T);
}

// ---------------------------------------------------------------------------
// Samurai
// ---------------------------------------------------------------------------

void UMXNameThemeManager::RegisterSamuraiTheme()
{
    FMXNameThemeData T;
    T.ThemeName = TEXT("Samurai");
    T.Description = TEXT("Honour, steel, and cherry blossoms. The way of the warrior.");
    T.UnlockCondition = TEXT("Have a robot survive 25 consecutive runs.");
    T.bUnlocked = false;
    T.TitlePrefix = TEXT("Honourable");

    T.FirstNames = {
        TEXT("Kumo"),     TEXT("Hoshi"),    TEXT("Kaze"),     TEXT("Yuki"),     TEXT("Sora"),
        TEXT("Rin"),      TEXT("Tsubame"),  TEXT("Akira"),    TEXT("Ishi"),     TEXT("Mizu"),
        TEXT("Tsuki"),    TEXT("Hinata"),   TEXT("Sakura"),   TEXT("Tetsu"),    TEXT("Raiden"),
        TEXT("Kaede"),    TEXT("Suzu"),     TEXT("Tora"),     TEXT("Fuji"),     TEXT("Nami"),
        TEXT("Umi"),      TEXT("Hayate"),   TEXT("Kiri"),     TEXT("Shiro"),    TEXT("Ren"),
        TEXT("Kai"),      TEXT("Yoru"),     TEXT("Haru"),     TEXT("Aki"),      TEXT("Natsu"),
    };

    T.UniversalSurnames = {
        TEXT("Hayabusa"),   TEXT("Kazehana"),  TEXT("Tsukimori"), TEXT("Ishikawa"),
        TEXT("Kurosawa"),   TEXT("Fujimoto"),  TEXT("Yukimura"),  TEXT("Takahashi"),
        TEXT("Shimizu"),    TEXT("Yamaguchi"),
    };
    T.ScoutSurnames = {
        TEXT("Kazemaru"),   TEXT("Hayakaze"),  TEXT("Tsubameki"), TEXT("Kageshiro"),
        TEXT("Mizuhara"),   TEXT("Kurogane"),  TEXT("Hoshikami"), TEXT("Suzukaze"),
    };
    T.GuardianSurnames = {
        TEXT("Tetsumaru"),  TEXT("Ishigami"),  TEXT("Iwakura"),   TEXT("Kaneshiro"),
        TEXT("Yamaoka"),    TEXT("Torishima"), TEXT("Hagane"),    TEXT("Tatetsu"),
    };
    T.EngineerSurnames = {
        TEXT("Haguruma"),   TEXT("Karakuri"),  TEXT("Tanegashima"),TEXT("Hasami"),
        TEXT("Tokei"),      TEXT("Kikkai"),    TEXT("Sarugaku"),  TEXT("Daikuma"),
    };

    T.TitleAliases = {
        {TEXT("Scorched"),       TEXT("Ember-Scarred")},
        {TEXT("Firewalker"),     TEXT("Flame Dancer")},
        {TEXT("Frostbitten"),    TEXT("Snow-Touched")},
        {TEXT("Grounded"),       TEXT("Earth-Bound")},
        {TEXT("Lucky"),          TEXT("Fortune-Blessed")},
        {TEXT("Untouchable"),    TEXT("Wind-Kissed")},
        {TEXT("Veteran"),        TEXT("Seasoned")},
        {TEXT("Legend"),         TEXT("Living Legend")},
        {TEXT("Immortal"),       TEXT("Eternal")},
        {TEXT("Haunted"),        TEXT("Spirit-Marked")},
        {TEXT("Witness"),        TEXT("All-Seeing")},
        {TEXT("On A Roll"),      TEXT("Favoured")},
        {TEXT("Unstoppable"),    TEXT("Invincible")},
        {TEXT("Scarred"),        TEXT("Battle-Marked")},
    };

    ThemeRegistry.Add(ENameTheme::Samurai, T);
}

// ---------------------------------------------------------------------------
// Sci-Fi
// ---------------------------------------------------------------------------

void UMXNameThemeManager::RegisterSciFiTheme()
{
    FMXNameThemeData T;
    T.ThemeName = TEXT("Sci-Fi");
    T.Description = TEXT("Starships, plasma cores, and designation codes. The future is now.");
    T.UnlockCondition = TEXT("Deploy 500 total robots.");
    T.bUnlocked = false;
    T.TitlePrefix = TEXT("Commander");

    T.FirstNames = {
        TEXT("Nova"),     TEXT("Quark"),    TEXT("Nebula"),   TEXT("Ion"),      TEXT("Vex"),
        TEXT("Photon"),   TEXT("Cosmo"),    TEXT("Orbital"),  TEXT("Flux"),     TEXT("Helix"),
        TEXT("Zenith"),   TEXT("Vertex"),   TEXT("Tau"),      TEXT("Sigma"),    TEXT("Omega"),
        TEXT("Delta"),    TEXT("Cipher"),   TEXT("Vector"),   TEXT("Matrix"),   TEXT("Prism"),
        TEXT("Axis"),     TEXT("Pulsar"),   TEXT("Void"),     TEXT("Quantum"),  TEXT("Aether"),
        TEXT("Nano"),     TEXT("Probe"),    TEXT("Stellar"),  TEXT("Eclipse"),  TEXT("Drift"),
    };

    T.UniversalSurnames = {
        TEXT("Starborn"),   TEXT("Voidwalker"), TEXT("Nullspace"),  TEXT("Deepfield"),
        TEXT("Lightspeed"), TEXT("Darkstar"),   TEXT("Hyperdrive"), TEXT("Singularity"),
        TEXT("Warpcore"),   TEXT("Eventline"),
    };
    T.ScoutSurnames = {
        TEXT("Pathfinder"), TEXT("Deepprobe"),  TEXT("Scanfield"), TEXT("Rangefinder"),
        TEXT("Longrange"),  TEXT("Sweepline"),  TEXT("Farpoint"),  TEXT("Beacon"),
    };
    T.GuardianSurnames = {
        TEXT("Shieldcore"), TEXT("Bulkplate"),  TEXT("Deflector"), TEXT("Hullguard"),
        TEXT("Chromaplate"),TEXT("Ironfield"),  TEXT("Barrier"),   TEXT("Fortress"),
    };
    T.EngineerSurnames = {
        TEXT("Fusioncore"), TEXT("Gridlock"),   TEXT("Circuitbend"),TEXT("Nanobuild"),
        TEXT("Solderpoint"),TEXT("Hackline"),   TEXT("Overclocked"),TEXT("Debugger"),
    };

    T.TitleAliases = {
        {TEXT("Scorched"),       TEXT("Plasma-Burned")},
        {TEXT("Firewalker"),     TEXT("Thermoshield")},
        {TEXT("Frostbitten"),    TEXT("Cryo-Locked")},
        {TEXT("Grounded"),       TEXT("Magnetically Sealed")},
        {TEXT("Lucky"),          TEXT("Probability Anomaly")},
        {TEXT("Untouchable"),    TEXT("Phase-Shifted")},
        {TEXT("Veteran"),        TEXT("Senior Officer")},
        {TEXT("Legend"),         TEXT("Fleet Admiral")},
        {TEXT("Immortal"),       TEXT("Perpetual")},
        {TEXT("Haunted"),        TEXT("Ghost in the Shell")},
        {TEXT("Witness"),        TEXT("Sensor Array")},
        {TEXT("On A Roll"),      TEXT("Momentum Drive")},
        {TEXT("Unstoppable"),    TEXT("Juggernaut")},
        {TEXT("Scarred"),        TEXT("Hull-Breached")},
    };

    ThemeRegistry.Add(ENameTheme::SciFi, T);
}

// ---------------------------------------------------------------------------
// Mythic
// ---------------------------------------------------------------------------

void UMXNameThemeManager::RegisterMythicTheme()
{
    FMXNameThemeData T;
    T.ThemeName = TEXT("Mythic");
    T.Description = TEXT("Gods, beasts, and heroes. Names from the oldest stories.");
    T.UnlockCondition = TEXT("Have a robot earn 3 titles.");
    T.bUnlocked = false;
    T.TitlePrefix = TEXT("The Mighty");

    T.FirstNames = {
        TEXT("Fenris"),   TEXT("Helios"),   TEXT("Nyx"),      TEXT("Atlas"),    TEXT("Chimera"),
        TEXT("Griffin"),  TEXT("Selene"),   TEXT("Orion"),    TEXT("Hydra"),    TEXT("Cerberus"),
        TEXT("Titan"),    TEXT("Valkyrie"), TEXT("Phoenix"),  TEXT("Kraken"),   TEXT("Minotaur"),
        TEXT("Sphinx"),   TEXT("Gorgon"),   TEXT("Centaur"),  TEXT("Banshee"),  TEXT("Wyrm"),
        TEXT("Roc"),      TEXT("Djinn"),    TEXT("Golem"),    TEXT("Manticore"),TEXT("Sylph"),
        TEXT("Cyclops"),  TEXT("Siren"),    TEXT("Wyvern"),   TEXT("Drake"),    TEXT("Basilisk"),
    };

    T.UniversalSurnames = {
        TEXT("Thunderborn"),TEXT("Stormforge"), TEXT("Dawnbringer"),TEXT("Nightfall"),
        TEXT("Worldbreaker"),TEXT("Starshaper"),TEXT("Fateweaver"), TEXT("Oathkeeper"),
        TEXT("Runecarver"), TEXT("Soulforged"),
    };
    T.ScoutSurnames = {
        TEXT("Windrunner"), TEXT("Swiftwing"),  TEXT("Moonstrider"),TEXT("Shadowleap"),
        TEXT("Stormrider"), TEXT("Skyborne"),   TEXT("Galefoot"),   TEXT("Nightwhisper"),
    };
    T.GuardianSurnames = {
        TEXT("Stoneheart"),  TEXT("Ironvow"),   TEXT("Shieldmaiden"),TEXT("Mountainkeep"),
        TEXT("Earthbound"),  TEXT("Warshield"), TEXT("Doomguard"),   TEXT("Wallbreaker"),
    };
    T.EngineerSurnames = {
        TEXT("Forgecaster"), TEXT("Runesmith"), TEXT("Hammerbound"), TEXT("Anvilsong"),
        TEXT("Chainweaver"), TEXT("Steelwhisper"),TEXT("Bellows"),   TEXT("Sparkwright"),
    };

    T.TitleAliases = {
        {TEXT("Scorched"),       TEXT("Fire-Forged")},
        {TEXT("Firewalker"),     TEXT("Flame-Blessed")},
        {TEXT("Frostbitten"),    TEXT("Winter-Kissed")},
        {TEXT("Grounded"),       TEXT("Earth-Rooted")},
        {TEXT("Lucky"),          TEXT("Fate-Favoured")},
        {TEXT("Untouchable"),    TEXT("Divine")},
        {TEXT("Veteran"),        TEXT("Elder")},
        {TEXT("Legend"),         TEXT("Legendary")},
        {TEXT("Immortal"),       TEXT("Eternal")},
        {TEXT("Haunted"),        TEXT("Doom-Marked")},
        {TEXT("Witness"),        TEXT("All-Seeing")},
        {TEXT("On A Roll"),      TEXT("Destiny-Bound")},
        {TEXT("Unstoppable"),    TEXT("Godslayer")},
        {TEXT("Scarred"),        TEXT("Battle-Blessed")},
    };

    ThemeRegistry.Add(ENameTheme::Mythic, T);
}
