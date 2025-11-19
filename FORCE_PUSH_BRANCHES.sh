#!/bin/bash
# This script force-pushes all the resolved branches to remote
# Run this with appropriate permissions to update the remote branches

branches=(
"claude/add-export-tempo-tests-01FVML4Hh1gYmbEuh9fhN4aH"
"claude/add-metronome-tempo-01KwZTwHSCCnxTNcTFZxrvPi"
"claude/arranger-pianoroll-integration-01F1R2eoUP9dGPDkXL1g9sPM"
"claude/arranger-timeline-ui-01THxjSmKv49rXp8zeovZj7B"
"claude/audio-recording-pipeline-0174P688TeBsY7Me92bDP6hh"
"claude/audit-unimplemented-code-011CUzqGbRshVH3fjHhBrRuT"
"claude/automation-lane-ui-0113FEq3c9UbWUCskmFKQhgs"
"claude/clip-synchronizer-bridge-01Vu5Git4Rd3nbWjUXAN8ot6"
"claude/command-api-coverage-016QmhwVf54YP3k3LHiuBpMv"
"claude/consolidate-main-011CV34SnbPLX34Cr2HUouKX"
"claude/dev-docs-architecture-01W58j1eknjb1NutMMb8x8VF"
"claude/engine-smoke-tests-01D3pGGij5rtPXMi2DUT9su3"
"claude/extend-commandapi-plugins-01MUiKgkxAPaGpAwRCeNTsge"
"claude/fix-export-buffer-size-0181vZVxy5RhqvCRDMKAcNEz"
"claude/fix-vst3-pr-conflicts-01SLRDbfNZmwF8pBQFUiuNuQ"
"claude/fix-wav-export-silence-01188gkKfHUKrJZkuUpv8hhC"
"claude/implement-wav-export-01Ucfh8c7qwmnFA22jXLGYYc"
"claude/integrate-newbase-cherrypick-011CV37jKJshcLP9VNMGNBoP"
"claude/juce-native-refactor-01B7w3xRPCy2nVYfdLA2fSVq"
"claude/juce8-zenith-branch-audit-011CV37jKJshcLP9VNMGNBoP"
"claude/midi-to-plugins-015Fjen8QeBNsRJd6X3udJBU"
"claude/mixer-view-ui-01GA1meAh7N1DFhTgeByGiV7"
"claude/phase-10-mixer-mvp-01WxSjeomQNr88ygo2SqgaMb"
"claude/phase-11-mixer-engine-meters-01UB8MsPUEhKgFunCPhovUoj"
"claude/phase-12-recording-ux-track-types-01669qZVTBk3LfXdnPnZJnED"
"claude/phase-14-automation-lanes-ui-01PKTe5asTBbBAuqHfcNbid1"
"claude/phase-9-arranger-clip-editing-01GazSp5uZX5BpQXfhazqfWg"
"claude/phase-implementation-pending-01Xjscr91zMSpMH2HMU9pfCi"
"claude/phase-implementation-session-013BQpzFJEqM9wGkuLRao4xa"
"claude/phase-implementation-worker-01Ltm76dGTTEebFnP62ueNYo"
"claude/phase-implementation-zenith-01FmPQgzPaCEjpMdB1om7XVc"
"claude/phase8-midi-valuetree-undo-012GWM4quckuDmZDbLvShY2u"
"claude/piano-roll-editor-01Td3QSRQYppdvaRhtzhMbA9"
"claude/plugin-host-core-011SPMbQhtYkJa6Mp7FgHwgg"
"claude/plugin-scanner-browser-01XB2K7yF9C4Wt9JZ7ZqnmQy"
"claude/recording-export-pipeline-01QS9rNokbDW8o8byDskDKTq"
"claude/remove-legacy-stacks-011CV34SnbPLX34Cr2HUouKX"
"claude/replace-daw-stubs-011CUzrNFmDz3bGQw691AquP"
"claude/setup-recording-engine-0192j4SNwY75GwmYR6sqKXyq"
"claude/track-header-ui-018e6JzHN1CGAenNrLpfszzs"
"claude/v01-core-model-011CV34SnbPLX34Cr2HUouKX"
"claude/verify-windows-docs-017rX1Widqxb9yUcgpxJh7kS"
"claude/vst3-plugin-hosting-mvp-01UNm2b4HkV6KPhYnLyiJv3M"
"claude/w10-2-scheduler-011CV34SnbPLX34Cr2HUouKX"
"claude/w10-3-mixer-events-011CV37jKJshcLP9VNMGNBoP"
"claude/w10-3-mixer-hooks-011CV34SnbPLX34Cr2HUouKX"
)

echo "Force pushing all resolved branches..."
echo "WARNING: This will overwrite remote branches!"
echo ""
read -p "Continue? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 1
fi

success=0
failed=0

for branch in "${branches[@]}"; do
    echo -n "Pushing $branch... "
    if git push --force-with-lease origin "$branch" 2>/dev/null; then
        echo "✓"
        ((success++))
    else
        echo "✗"
        ((failed++))
    fi
done

echo ""
echo "Results: $success succeeded, $failed failed"
