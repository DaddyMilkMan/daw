# Merge all PRs into master
# Based on PR list from GitHub

$prs = @(
    @{num=88; title="Phase 12: Recording UX & Track Types"},
    @{num=85; title="Claude/phase implementation pending"},
    @{num=84; title="Phase 15: Tempo Map + Markers v1"},
    @{num=82; title="Phase 14: Arranger Automation Lanes UI"},
    @{num=81; title="Claude/juce native refactor"},
    @{num=80; title="Claude/phase implementation worker"},
    @{num=79; title="Claude/automation lane UI"},
    @{num=78; title="Claude/recording export pipeline"},
    @{num=77; title="U4.1: beat-based clip & MIDI note model"},
    @{num=76; title="Add metronome and global tempo control"},
    @{num=75; title="Implement ClipSynchronizer"},
    @{num=74; title="U4.3: Piano Roll Editor"},
    @{num=73; title="docs: architecture and developer workflow"},
    @{num=70; title="Claude/implement wav export"},
    @{num=69; title="Implement Arranger Timeline UI"},
    @{num=65; title="feat: VST3 plugin scanner and browser UI"},
    @{num=63; title="Claude/setup recording engine"},
    @{num=56; title="feat: extend CommandAPI"},
    @{num=55; title="feat: integrate arranger and piano roll"},
    @{num=54; title="feat: implement WAV export system"},
    @{num=53; title="feat: engine smoke-test console"},
    @{num=52; title="feat: minimal per-track plugin hosting"},
    @{num=51; title="Add Mixer View UI"},
    @{num=50; title="docs: align architecture docs"},
    @{num=48; title="Implement Track Header UI"},
    @{num=47; title="chore: remove build artifacts"},
    @{num=45; title="Add merge plan documentation"},
    @{num=43; title="Add Windows installation docs"},
    @{num=42; title="Add tempo/BPM logic tests"}
)

Write-Host "Found $($prs.Count) PRs to merge"
Write-Host "Fetching all PR branches..."

# Fetch all branches
git fetch --all --prune

# Get list of all remote branches
$branches = git branch -r | Where-Object { $_ -notmatch "HEAD" -and $_ -match "origin/" }

Write-Host "`nAvailable branches:"
$branches | ForEach-Object { Write-Host $_.Trim() }
