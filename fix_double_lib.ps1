$cmakes = Get-ChildItem "apps\desktop\Source" -Recurse -Filter "CMakeLists.txt"
$skia = Get-Item "apps\desktop\cmake\SkiaManualIntegration.cmake"
$files = $cmakes + $skia

foreach ($file in $files) {
    if (!$file) { continue }
    $content = Get-Content $file.FullName -Raw
    if (!$content) { continue }
    
    $newContent = $content.Replace("_lib_lib", "_lib")
    
    if ($newContent -ne $content) {
        Set-Content $file.FullName $newContent
        Write-Host "Fixed $($file.FullName)"
    }
}
