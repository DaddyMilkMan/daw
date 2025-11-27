# Script to replace #include <JuceHeader.h> with explicit JUCE module includes
# This fixes IDE IntelliSense errors

$oldInclude = '#include <JuceHeader.h>'
$newIncludes = @'
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
'@

# Find all .h and .cpp files
$files = Get-ChildItem -Path "." -Include *.h,*.cpp -Recurse

$count = 0
foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    if ($content -match [regex]::Escape($oldInclude)) {
        $newContent = $content -replace [regex]::Escape($oldInclude), $newIncludes
        Set-Content -Path $file.FullName -Value $newContent -NoNewline
        Write-Host "Fixed: $($file.FullName)"
        $count++
    }
}

Write-Host "`nTotal files fixed: $count"
