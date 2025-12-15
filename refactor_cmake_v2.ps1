$cmakes = Get-ChildItem "apps\desktop\Source" -Recurse -Filter "CMakeLists.txt"
$skia = Get-Item "apps\desktop\cmake\SkiaManualIntegration.cmake"
$files = $cmakes + $skia

foreach ($file in $files) {
    if (!$file) { continue }
    $content = Get-Content $file.FullName -Raw
    if (!$content) { continue }

    # Use .Replace for literal replacement
    $newContent = $content.Replace("target_sources(ZenithDAW", "target_sources(ZenithDAW_lib")
    $newContent = $newContent.Replace("target_include_directories(ZenithDAW", "target_include_directories(ZenithDAW_lib")
    $newContent = $newContent.Replace("target_link_libraries(ZenithDAW", "target_link_libraries(ZenithDAW_lib")
    $newContent = $newContent.Replace("target_compile_definitions(ZenithDAW", "target_compile_definitions(ZenithDAW_lib")
    
    # Check for Skia script specific
    if ($file.Name -eq "SkiaManualIntegration.cmake") {
        # It might use simple ZenithDAW reference
        $newContent = $newContent.Replace("target_include_directories(ZenithDAW", "target_include_directories(ZenithDAW_lib")
        $newContent = $newContent.Replace("target_link_libraries(ZenithDAW", "target_link_libraries(ZenithDAW_lib")
    }

    if ($newContent -ne $content) {
        Set-Content $file.FullName $newContent
        Write-Host "Updated $($file.FullName)"
    }
}
