
function parse_version($filename) {
	$version_content = Get-Content $filename
	$version_lines = $version_content -split $newline
	$VERSION = $null
	$str = "#define";
	foreach ($line in $version_lines) {
		if ($line -match ("\s*" + $str + "\s+\S+\s+")) {
			$pos = $line.IndexOf($str);
			$pos += $str.Length;
			while ($line[$pos] -le 32) {
				++$pos;
			}
			while ($line[$pos] -gt 32) {
				++$pos;
			}
			$VERSION = $line.Substring($pos).Trim()
			if ($VERSION[0] -eq "`"") {
				$VERSION = $VERSION.Substring(1);
			}
			if ($VERSION[$VERSION.Length - 1] -eq "`"") {
				$VERSION = $VERSION.Substring(0, $VERSION.Length - 1);
			}
			return $VERSION;
		}
	}
	if (($null -eq $VERSION) -or (-not ($VERSION -is [String]))) {
		throw "Could not parse version."
	}
}

function replace_about_dlg($content_lines) {
	$inside_dialog = $false
	
	for ($i = 0; $i -lt $content_lines.Count; ++$i) {
		$line = $content_lines[$i];
		if ($line -match "\s*IDD_ABOUTBOX DIALOGEX") {
			$inside_dialog = $true;
		} elseif ($inside_dialog -and ($line -match ("\s*LTEXT\s+`"\S+\s+[^`"\s]+`""))) {
			$pos1 = $line.IndexOf("`"");
			++$pos1;  # skip quote
			while ($line[$pos1] -gt 32) {
				++$pos1;
			}
			while ($line[$pos1] -le 32) {
				++$pos1;
			}
			$pos2 = $line.IndexOf("`"", $pos1);
			$old_version = $line.Substring($pos1, $pos2 - $pos1);
			if ($old_version -ne $VERSION) {
				$content_lines[$i] = $line.Substring(0, $pos1) + $VERSION + $line.Substring($pos2);
				return $true;
			}
			return $false;
		}
	}
	
	return $false;
}

function replace_app_title($content_lines) {
	for ($i = 0; $i -lt $content_lines.Count; ++$i) {
		$line = $content_lines[$i];
		if ($line -match "\s*IDS_APP_TITLE\s+`"\S+\s+[^`"\s]+`"") {
			$parsing_step = 0
			for ($pos = $line.IndexOf("`"") + 1; $pos -lt $line.Length; ++$pos) {
				$chr = $line[$pos];
				if ($parsing_step -eq 0) {
					if ($chr -le 32) {
						$parsing_step = 1;
					}
				} elseif ($parsing_step -eq 1) {
					if ($chr -gt 32) {
						$parsing_step = 2;
						--$pos;
					}
				} else {
					# pos should be pointing to the start of the old VERSION
					$pos2 = $line.IndexOf("`"", $pos);
					$old_version = $line.Substring($pos, $pos2 - $pos);
					if ($old_version -ne $VERSION) {
						$content_lines[$i] = $line.Substring(0, $pos) + $VERSION + $line.Substring($pos2);
						return $true;
					}
					return $false;
				}
			}
			return $false;
		}
	}
	
	return $false;
}

function replace_file_and_product_versions($content_lines, $new_version) {
	$need_write = $false;
	$new_version_with_dots = $new_version.Replace(",", ".");
	$new_version_with_commas = $new_version.Replace(".", ",");
	
	for ($i = 0; $i -lt $content_lines.Count; ++$i) {
		$line = $content_lines[$i];
		$str1 = "FILEVERSION";
		$str2 = "PRODUCTVERSION";
		if ((
				$line -match "\s*VALUE\s+`"FileVersion`",\s*`"[^`"\s]+`""
			) -or (
				$line -match "\s*VALUE\s+`"ProductVersion`",\s*`"[^`"\s]+`""
			)
		) {
			$pos = $line.IndexOf("`"");
			$pos = $line.IndexOf("`"", $pos + 1);
			$pos = $line.IndexOf("`"", $pos + 1);
			$pos2 = $line.IndexOf("`"", $pos + 1);
			$old_version = $line.Substring($pos + 1, $pos2 - $pos - 1);
			if ($old_version -ne $new_version_with_dots) {
				$need_write = $true;
				$content_lines[$i] = $line.Substring(0, $pos + 1) + $new_version_with_dots + $line.Substring($pos2);
			}
		} elseif ((
				$line -match ("\s*" + $str1 + "\s+\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+")
			) -or (
				$line -match ("\s*" + $str2 + "\s+\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+")
			)
		) {
			$pos = 0;
			while ($line[$pos] -le 32) {
				++$pos;
			}
			while ($line[$pos] -gt 32) {
				++$pos;
			}
			$old_version = $line.Substring($pos).Replace(" ", "").Replace($tab, "");
			if ($old_version -ne $new_version_with_commas) {
				$need_write = $true;
				$content_lines[$i] = $line.Substring(0, $pos) + " " + $new_version_with_commas;
			}
		}
	}
	
	return $need_write;
}
