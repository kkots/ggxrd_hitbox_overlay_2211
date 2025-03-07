$content = Get-Content "ggxrd_hitbox_patcher.rc"
$newline = "`
"
$tab = "	";

. "..\common\prebuild.ps1"

$VERSION = parse_version("PatcherVersion.h")

$content_lines = $content -split $newline
$need_write = $false

$new_version = $VERSION.Substring(2) + ".0.0.0";

if (replace_file_and_product_versions $content_lines $new_version) {
	$need_write = $true;
}

if ($need_write) {
	Set-Content -Value ($content_lines -join $newline) "ggxrd_hitbox_patcher.rc" -Encoding Unicode
}
