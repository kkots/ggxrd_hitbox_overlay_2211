$content = Get-Content "ggxrd_hitbox_overlay.rc"
$newline = "`
"
$tab = "	";

. "..\common\prebuild.ps1"

$VERSION = parse_version("Version.h")

$content_lines = $content -split $newline
$need_write = $false

$dotpos = $VERSION.IndexOf(".");
$new_version = $VERSION.Substring(0, $dotpos) + ",0," + $VERSION.Substring($dotpos + 1) + ",0";

if (replace_file_and_product_versions $content_lines $new_version) {
	$need_write = $true;
}

if ($need_write) {
	Set-Content -Value ($content_lines -join $newline) "ggxrd_hitbox_overlay.rc" -Encoding Unicode
}

cd ..
. ".\regenerate_ini_and_update_readme.ps1"
