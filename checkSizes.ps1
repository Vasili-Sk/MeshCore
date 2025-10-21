# PowerShell script to run arm-none-eabi-size on object files included in an ELF
$rootFolder = ".pio\build\tinySolarNode_repeater\"
$elfFile = $rootFolder + "firmware.elf"
$mapFile = $rootFolder + "firmware.map"
$outputFile = "object_files.txt"

# Check if map file exists
if (-not (Test-Path $mapFile)) {
    Write-Output "Error: Map file $mapFile not found!"
    exit
}

# Extract object files from the map file
Get-Content $mapFile | Where-Object { $_ -match "\.o$" } | ForEach-Object { $_.Split()[-1] } | Sort-Object | Get-Unique > $outputFile

# Check if any object files were found
if (-not (Test-Path $outputFile) -or (Get-Content $outputFile).Count -eq 0) {
    Write-Output "No object files found in $mapFile"
    Remove-Item $outputFile -ErrorAction SilentlyContinue
    exit
}

# Initialize variables
$headerPrinted = $false
$header = ""

# Run arm-none-eabi-size on each object file
Write-Output "Memory usage for object files included in $elfFile"
Get-Content $outputFile | ForEach-Object {
    $objFile = $_
    if (Test-Path $objFile) {
        # Capture the output of arm-none-eabi-size
        $sizeOutput = & arm-none-eabi-size $objFile | Out-String -Stream
        if ($sizeOutput) {
            if (-not $headerPrinted) {
                # Print the header from the first valid output
                $header = $sizeOutput | Where-Object { $_ -match "text\s+data\s+bss\s+dec\s+hex\s+filename" }
                Write-Output $header
                $headerPrinted = $true
            }
            # Print only the data line (skip the header)
            $dataLine = $sizeOutput | Where-Object { $_ -match "\d+\s+\d+\s+\d+\s+\d+\s+[0-9a-f]+\s+$objFile" }
            Write-Output $dataLine
        }
    } else {
        Write-Output "Warning: Object file $objFile not found!"
    }
}

# Clean up
Remove-Item $outputFile -ErrorAction SilentlyContinue
Pause