$VERSION = npx semantic-release --dry-run | Select-String -Pattern "The next release version is" | ForEach-Object { $_.ToString() -replace '.*The next release version is ([0-9.]+).*', '$1' }
Write-Host "Version: $VERSION"
Write-Output "::set-output name=version::$version"
Set-Content -Path ./version -Value "# current version of millennium installer`nv$VERSION"

# Commit to the repository as Github Actions

git config --local user.name "github-actions[bot]"
git config --local user.email "github-actions[bot]@users.noreply.github.com"
git add version
git commit -m "chore: update version to $VERSION"