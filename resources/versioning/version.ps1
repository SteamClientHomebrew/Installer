Push-Location resources/versioning
$VERSION = npx semantic-release --dry-run | Select-String -Pattern "The next release version is" | ForEach-Object { $_.ToString() -replace '.*The next release version is ([0-9.]+).*', '$1' }
Pop-Location

Write-Host "Version: $VERSION"
Write-Output "::set-output name=version::$version"
Set-Content -Path ./resources/versioning/version -Value "# current version of millennium installer`nv$VERSION"

# Commit to the repository as Github Actions

git config --local user.name "github-actions[bot]"
git config --local user.email "github-actions[bot]@users.noreply.github.com"
git add resources/versioning/version
git commit -m "chore: update version to $VERSION"
