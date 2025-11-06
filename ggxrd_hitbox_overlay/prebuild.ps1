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

function replace_absolute_paths_with_relative($content_lines) {
    $need_write = $false;
    # $PSCommandPath will point to the solution root, will include project's folder and the name of the .ps1 script
    # remove ps1 script's name
	$pos = $PSCommandPath.LastIndexOf("\");
    if (($pos -eq -1) -and ($pos -gt 0)) {
        return;
    }
    # remove project folder
    $pos = $PSCommandPath.LastIndexOf("\", $pos - 1);
    # .rc file doubles all slashes, so we double too
    $project_abs_path = ($PSCommandPath.Substring(0, $pos) -replace '\\','\\');
	for ($i = 0; $i -lt $content_lines.Count; ++$i) {
		$line = $content_lines[$i];
		if ($line -match '^[\w\d_]+\s+[\w\d_]+\s+"\w:\\\\[^"]*"$') {
            # maybe there's a way to get capturing groups using $Matches, but right now I forgot, and it's easier to write all this than to remember
            $pos = 0;
            while (($pos -lt $line.Length) -and ($line[$pos] -match '[\w\d_]')) {
                ++$pos;
            }
            while (($pos -lt $line.Length) -and ($line[$pos] -match '\s')) {
                ++$pos;
            }
            while (($pos -lt $line.Length) -and ($line[$pos] -match '[\w\d_]')) {
                ++$pos;
            }
            while (($pos -lt $line.Length) -and ($line[$pos] -match '\s')) {
                ++$pos;
            }
            if (($pos -lt $line.Length) -and ($line[$pos] -eq '"')) {
                $pos_start = $pos;
                $pos_end = $line.IndexOf('"', $pos + 1);
                if ($pos_end -ne -1) {
                    $line_path = $line.Substring($pos_start + 1, $pos_end - $pos_start - 1);
                    if ($line_path.Substring(0, $project_abs_path.Length) -eq $project_abs_path) {
                        # this .rc file is of the project. From the project we get to the solution folder using .., so that's why we replace the solution folder with ..
                        $content_lines[$i] = $line.Substring(0, $pos_start + 1) + ".." + $line.Substring($pos_start + 1 + $project_abs_path.Length)
                        $need_write = $true;
                    }
                }
            }
		}
	}
    return $need_write;
}

if (replace_absolute_paths_with_relative $content_lines) {
    $need_write = $true;
}

if ($need_write) {
	Set-Content -Value ($content_lines -join $newline) "ggxrd_hitbox_overlay.rc" -Encoding Unicode
}

cd ..
. ".\regenerate_ini_and_update_readme.ps1"
