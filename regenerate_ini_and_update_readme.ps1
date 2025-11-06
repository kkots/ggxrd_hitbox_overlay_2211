# This script only regenerates the INI and updates README.md if the latest date of modification of any of the following files:
# 1) ggxrd_hitbox_overlay\KeyDefinitions.h,
# 2) ggxrd_hitbox_overlay\SettingsDefinitions.h,
# 3) ggxrd_hitbox_overlay\SettingsTopCommentDefinition.cpp
# is greater than the date recorded at regenerate_ini_and_update_readme__last_date.txt. This file is excluded from git.

# The README that we're going to update will be the root README of the entire solution.
# The INI file is located in the root as well.
# First we will update the INI, then the README.

$ErrorActionPreference = "Stop"

class ParseCStringResult {
    [int]$pos  # the encountered character that is after the string that isn't a comment or a ". May point to the end of the whole txt
    [string]$str  # the parsed string with everything unescaped and all outer quotes removed
}

$dtFormat = "yyyy-MM-ddThh:mm:ss";

# $pos must point to an opening "
# unescapes everything
# removes comments
# stops at a # (preprocessor macro) or any character that is not inside a string and isn't a comment or a "
function parseCString($txt, $pos) {
    $inQuotes= $false;
    $builder = New-Object System.Text.StringBuilder;
    $result = New-Object ParseCStringResult;
    $quitPos = -1;
    for ($i = $pos; $i -lt $txt.Length; ++$i) {
        [char]$c = $txt.Chars($i);
        if (-not $inQuotes) {
            if ($c -eq 47) { # /
                if (($i + 1) -lt $txt.Length) {
                    [char]$nextC = $txt.Chars($i + 1);
                    if ($nextC -eq 47) {  # /
                        $nextPos = $txt.IndexOf(10, $i);  # \n
                        if ($nextPos -eq -1) {
                            break;
                        } else {
                            $i = $nextPos;
                            continue;
                        }
                    } elseif ($nextC -eq 42) {  # *
                        $nextPos = $txt.IndexOf("*/", $i);
                        if ($nextPos -eq -1) {
                            break;
                        } else {
                            $i = $nextPos + 1;
                            continue;
                        }
                    } else {
                        $quitPos = $i;
                        break;
                    }
                } else {
                    $quitPos = $i;
                    break;
                }
            } elseif ($c -eq 34) {  # "
                $inQuotes = $true;
                continue;
            } elseif ($c -le 32) {  # whitespace
                continue;
            } else {
                $quitPos = $i;
                break;
            }
        } elseif ($c -eq 92) {  # \
            ++$i;
            if ($i -lt $txt.Length) {
                [char]$nextC = $txt.Chars($i);
                if ($nextC -eq 110) {  # n
                    $builder.Append([char]10);
                } elseif ($nextC -eq 116) {  # t
                    $builder.Append([char]9);
                } elseif ($nextC -eq 34) {  # "
                    $builder.Append([char]34);
                } elseif ($nextC -eq 92) {  # \
                    $builder.Append([char]92);
                }
            }
            continue;
        } elseif ($c -eq 34) {  # "
            $inQuotes = $false;
        } else {
            $builder.Append($c);
        }
    }
    if ($quitPos -ne -1) {
        $result.pos = $quitPos;
    } else {
        $result.pos = $txt.Length;
    }
    $result.str = $builder.ToString();
    return $result;
}

function task() {
	
    if (-not (Test-Path "README.md")) {
        Write-Error "README.md not found.";
        return;
    }

    $lastDate = $null;
    if (Test-Path "regenerate_ini_and_update_readme__last_date.txt") {
        $lastDate = Get-Content "regenerate_ini_and_update_readme__last_date.txt"
        if ($null -eq $lastDate) {
            // the file is empty
        } elseif ($lastDate -is [array]) {
            // the file has multiple lines
            $lastDate = $lastDate[0];
        } elseif (-not ($lastDate -is [string])) {
            $lastDate = $null;
        }
        if ($null -ne $lastDate) {
            $lastDate = [System.DateTime]$lastDate;
        }
    }

    $currentDate = $null;
    $currentDates = @($null, $null, $null);
    $currentDates[0] = (Get-Item "ggxrd_hitbox_overlay\KeyDefinitions.h").LastWriteTime;
    $currentDates[1] = (Get-Item "ggxrd_hitbox_overlay\SettingsDefinitions.h").LastWriteTime;
    $currentDates[2] = (Get-Item "ggxrd_hitbox_overlay\SettingsTopCommentDefinition.cpp").LastWriteTime;
    for ($i = 0; $i -lt 3; ++$i) {
        $dt = $currentDates[$i];
        if (($null -eq $currentDate) -or ($dt -gt $currentDate)) {
            $currentDate = $dt;
        }
    }
    if ($null -ne $currentDate) {
        $currentDate = $currentDate.AddMilliseconds(-$currentDate.Millisecond);
    }
    if (($null -ne $currentDate) -and ($null -ne $lastDate) -and (
            [Math]::Floor([double]$lastDate.Ticks / [double]10000.0) -eq [Math]::Floor([double]$currentDate.Ticks / [double]10000.0)
    ) -and (Test-Path "ggxrd_hitbox_overlay.ini")) {
        return;
    }

    # \r char gets removed
	$keyDefinitions = [string]::Join("`n", (Get-Content "ggxrd_hitbox_overlay\KeyDefinitions.h"));
	$settingsDefinitions = [string]::Join("`n", (Get-Content "ggxrd_hitbox_overlay\SettingsDefinitions.h"))
    $settingsTopComment = [string]::Join("`n", (Get-Content "ggxrd_hitbox_overlay\SettingsTopCommentDefinition.cpp"))
	
	$searchStr = "#define keyEnum \";
	$pos = $keyDefinitions.IndexOf($searchStr)
	if ($pos -eq -1) {
		Write-Error ($searchStr + " not found in KeyDefinitions.h")
		return;
	}
	
	$pos += $searchStr.Length;
    
    $keyNames = [System.Collections.Generic.List[string]]::new();

    while ($true) {
        $pos = $keyDefinitions.IndexOf(34, $pos);  # "
        if ($pos -eq -1) {
            break;
        }
        ++$pos;
        $posStart = $pos;
        $pos = $keyDefinitions.IndexOf(34, $pos);  # "
        if ($pos -eq -1) {
            break;
        }

        $posEnd = $pos;
        $pos = $keyDefinitions.IndexOf(10, $pos + 1);  # \n

        $keyNames.Add($keyDefinitions.Substring($posStart, $posEnd - $posStart));

        if ($pos -eq -1) {
            break;
        }
        ++$pos;
    }

    $keyNames = [string]::Join(", ", $keyNames);
    
    
    [System.Text.RegularExpressions.Regex]$regex = New-Object System.Text.RegularExpressions.Regex("const\s+char\s*\*\s*settingsTopComment\s*=\s*`"")
    [System.Text.RegularExpressions.Match]$match = $regex.Match($settingsTopComment);
    if (-not $match.Success) {
        Write-Error "const char* settingsTopComment definition not found in SettingsTopCommentDefinition.cpp";
        return;
    }
    $parseCStringResult = parseCString $settingsTopComment ($match.Index + $match.Length - 1)
    if ($parseCStringResult.pos -eq $settingsTopComment.Length) {
        Write-Error "Couldn't find the end of the first part of settingsTopComment in SettingsTopCommentDefinition.cpp";
        return;
    }
    $settingsTopCommentPart1 = $parseCStringResult.str;

    $searchStr = "keyEnum";
    if (
        (
            ($parseCStringResult.pos + $searchStr.Length) -le $settingsTopComment.Length
        ) -and ($settingsTopComment.Substring($parseCStringResult.pos, $searchStr.Length) -eq $searchStr)
    ) {
        $pos = $parseCStringResult.pos + $searchStr.Length;
        $pos = $settingsTopComment.IndexOf(34, $pos);  # "
        if ($pos -eq -1) {
            Write-Error "Couldn't find the continuation of the string declaration after the mention of keyEnum in settingsTopComment in SettingsTopCommentDefinition.cpp";
            return;
        } else {
            $parseCStringResult = parseCString $settingsTopComment $pos
            if ($parseCStringResult.pos -eq $settingsTopComment.Length) {
                Write-Error "Couldn't find the end of the second part of settingsTopComment in SettingsTopCommentDefinition.cpp";
                return;
            } elseif ($settingsTopComment.Chars($parseCStringResult.pos) -ne 59) {  # ;
                Write-Error "The end of the second part of settingsTopComment is not ; in SettingsTopCommentDefinition.cpp";
                return;
            } else {
                $settingsTopComment = $settingsTopCommentPart1 + $keyNames + $parseCStringResult.str;
            }
        }
    } else {
        Write-Error "Couldn't find the mention of keyEnum in settingsTopComment in SettingsTopCommentDefinition.cpp";
        return;
    }


    [System.Collections.Generic.List[string]]$newIni = New-Object System.Collections.Generic.List[string];
    foreach ($line in ($settingsTopComment -split "`n")) {
        $newIni.Add($line);
    }

    $lineStart = 0;
    $lineStartNext = 0;
    $pos = -1;
    $isFirst = $true;
    $parseCStringResult = New-Object ParseCStringResult
    $parseCStringResult.pos = $settingsDefinitions.Length;
    while ($lineStart -lt $settingsDefinitions.Length) {
        if ($isFirst) {
            $isFirst = $false;
        } else {
            $shortcutPos = -1;
            if ($pos -ne -1) {
                $shortcutPos = $pos;
            }
            if (
                ($parseCStringResult.pos -ne $settingsDefinitions.Length) -and (
                    ($shortcutPos -eq -1) -or (
                        $parseCStringResult.pos -gt $shortcutPos
                    )
                )
            ) {
                $shortcutPos = $parseCStringResult.pos;
            }
            if (($shortcutPos -ne -1) -and ($shortcutPos -gt $lineStart)) {
                $lineStart = $shortcutPos;
            }
            $lineStart = $settingsDefinitions.IndexOf(10, $lineStart);  # \n
            if ($lineStart -eq -1) {
                break;
            }
            ++$lineStart;
        }

        # skip initial whitespace on the line (maybe skip ahead multiple lines)
        while (($lineStart -lt $settingsDefinitions.Length) -and ($settingsDefinitions.Chars($lineStart) -le 32)) {  # whitespace
            ++$lineStart;
        }
        if ($lineStart -ge $settingsDefinitions.Length) {
            break;
        }

        # skip preprocessor macros
        if ($settingsDefinitions.Chars($lineStart) -eq 35) {  # #
            continue;
        }
        
        $nextLine = $settingsDefinitions.IndexOf(10, $lineStart);
        if ($nextLine -eq -1) {
            $nextLine = $settingsDefinitions.Length;
        }
        $openBracePos = $settingsDefinitions.IndexOf(40, $lineStart);  # (
        if (($openBracePos -eq -1) -or ($openBracePos -ge $nextLine)) {
            continue;
        }
        $pos = $openBracePos + 1;
        $macroName = $settingsDefinitions.Substring($lineStart, $openBracePos - $lineStart);
        $lineStart = $pos;
        if ($macroName -eq "settingsKeyCombo") {
            # find the , that is after the name
            $pos = $settingsDefinitions.IndexOf(44, $pos + 1);  # ,
            if ($pos -eq -1) {
                continue;
            }
            $name = $settingsDefinitions.Substring($openBracePos + 1, $pos - $openBracePos - 1);
            # find the opening " of the display name
            $pos = $settingsDefinitions.IndexOf(34, $pos + 1);  # "
            if ($pos -eq -1) {
                continue;
            }
            # parse the display name
            $parseCStringResult = parseCString $settingsDefinitions $pos
            if ($parseCStringResult.pos -eq $settingsDefinitions.Length) {
                continue;
            }
            # the comma after the display name
            $pos = $parseCStringResult.pos;
            if ($settingsDefinitions.Chars($pos) -ne 44) {  # ,
                continue;
            }
            # find the opening " of the default value
            $pos = $settingsDefinitions.IndexOf(34, $pos + 1);  # "
            if ($pos -eq -1) {
                continue;
            }
            # parse the default value
            $parseCStringResult = parseCString $settingsDefinitions $pos
            if ($parseCStringResult.pos -eq $settingsDefinitions.Length) {
                continue;
            }
            $defaultValue = $parseCStringResult.str;
            # the comma after the default value
            $pos = $parseCStringResult.pos;
            if ($settingsDefinitions.Chars($pos) -ne 44) {  # ,
                continue;
            }
            # find the opening " of the description
            $pos = $settingsDefinitions.IndexOf(34, $pos + 1);  # "
            if ($pos -eq -1) {
                continue;
            }
            # parse the description
            $parseCStringResult = parseCString $settingsDefinitions $pos
            if ($parseCStringResult.pos -eq $settingsDefinitions.Length) {
                continue;
            }
            if ($settingsDefinitions.Chars($parseCStringResult.pos) -ne 41) {  # )
                continue;
            }

            $newIni.Add("");
            foreach ($line in ($parseCStringResult.str -split "`n")) {
                $newIni.Add($line);
            }
            $newIni.Add($name + " = " + $defaultValue);

            # stop at the closing )
            $pos = $parseCStringResult.pos;

        } elseif ($macroName -eq "settingsField") {
            # find the , that is after the type
            $pos = $settingsDefinitions.IndexOf(44, $pos + 1);  # ,
            if ($pos -eq -1) {
                continue;
            }

            # parse the type
            $typename = $settingsDefinitions.Substring($openBracePos + 1, $pos - $openBracePos - 1);

            # skip the , after the type
            ++$pos;

            # skip the whitespace before the name
            while (($pos -lt $settingsDefinitions.Length) -and ($settingsDefinitions.Chars($pos) -le 32)) {  # whitespace
                ++$pos;
            }
            $nameStart = $pos;

            # find the , after the name
            $pos = $settingsDefinitions.IndexOf(44, $pos + 1);  # ,
            if ($pos -eq -1) {
                continue;
            }

            # parse the name
            $name = $settingsDefinitions.Substring($nameStart, $pos - $nameStart);

            # skip the whitespace before the default value
            ++$pos;
            while (($pos -lt $settingsDefinitions.Length) -and ($settingsDefinitions.Chars($pos) -le 32)) {  # whitespace
                ++$pos;
            }
            $valueStart = $pos;

            # find the , after the default value
            $pos = $settingsDefinitions.IndexOf(44, $pos + 1);  # ,
			if ($pos -eq -1) {
				continue;
			}

            # parse the value
            $value = $settingsDefinitions.Substring($valueStart, $pos - $valueStart);

            # transform the value to the INI format
            if ($typename -eq "ScreenshotPath") {
                if (($value.Length -ge 2) -and ($value.Chars(0) -eq 34) -and ($value.Chars($value.Length - 1) -eq 34)) {  # "
                    $valueParsed = parseCString $value 0
                    if ($valueParsed.pos -ne $value.Length) {
                        $value = $valueParsed.str;
                    } else {
                        $value = "";
                    }
                }
            } elseif ($typename -eq "float") {
                if (($value.Length -gt 0) -and (
                        ($value.Chars($value.Length - 1) -eq 70) -or (  # F
                            $value.Chars($value.Length - 1) -eq 102  # f
                        )
                    )
                ) {
                    $value = $value.Substring(0, $value.Length - 1);
                }
                if (($value.Length -gt 0) -and ($value.Chars($value.Length - 1) -eq 46)) {  # .
                    $value += "0";
                } elseif ($value.IndexOf(46) -eq -1) {  # .
                    $value += ".0";
                } elseif (($value.Length -gt 0) -and ($value.Chars(0) -eq 46)) {  # .
                    $value = "0" + $value;
                }
            } elseif ($typename -eq "HitboxList") {
            	$vPos = $value.IndexOf(34);  # "
            	if ($vPos -ne -1) {
            		$parseCStringResult = parseCString $value $vPos
				    $value = $parseCStringResult.str;
			    }
            } elseif (
            	(
            		$typename -eq "MoveList"
        		) -or (
        			$typename -eq "PinnedWindowList"
    			)
    		) {
            	$value = "";
            }

            # find the opening " of the display name
            $pos = $settingsDefinitions.IndexOf(34, $pos + 1);  # "
            if ($pos -eq -1) {
                continue;
            }

            # parse the display name
            $parseCStringResult = parseCString $settingsDefinitions $pos
            if ($parseCStringResult.pos -eq $settingsDefinitions.Length) {
                continue;
            }
            $pos = $parseCStringResult.pos;
            # should be a , after the display name
            if ($settingsDefinitions.Chars($pos) -ne 44) {  # ,
                continue;
            }
            # skip the comma
            ++$pos;
            
            # skip the whitespace before the section
            while (($pos -lt $settingsDefinitions.Length) -and ($settingsDefinitions.Chars($pos) -le 32)) {  # whitespace
                ++$pos;
            }

            # section can be either a quoted string or a variable's identifier
            if (($pos -lt $settingsDefinitions.Length) -and ($settingsDefinitions.Chars($pos) -eq 34)) {  # "
                # section is a string. Parse it
                $parseCStringResult = parseCString $settingsDefinitions $pos
                if ($parseCStringResult.pos -eq $settingsDefinitions.Length) {
                    continue;
                }
                $pos = $parseCStringResult.pos;
                # should be a comma after the string
                if ($settingsDefinitions.Chars($pos) -ne 44) {  # ,
                    continue;
                }
            } else {
                # find the , after the identifier
                $pos = $settingsDefinitions.IndexOf(44, $pos + 1);  # ,
                if ($pos -eq -1) {
                    continue;
                }
            }

            # find the opening " of the description
            $pos = $settingsDefinitions.IndexOf(34, $pos + 1);  # "
            if ($pos -eq -1) {
                continue;
            }
            # parse the description
            $parseCStringResult = parseCString $settingsDefinitions $pos
            if ($parseCStringResult.pos -eq $settingsDefinitions.Length) {
                continue;
            }
            $desc = $parseCStringResult.str;
            # after the description, we're either at a ) or at a comma
            $pos = $parseCStringResult.pos;
            if ($settingsDefinitions.Chars($pos) -eq 41) {  # )
                $inlineComment = "";
            } elseif ($settingsDefinitions.Chars($pos) -eq 44) {  # ,
                # find the opening " of the inline comment
                $pos = $settingsDefinitions.IndexOf(34, $pos + 1);  # "
                if ($pos -eq -1) {
                    continue;
                }
                # parse the inline comment
                $parseCStringResult = parseCString $settingsDefinitions $pos
                if ($parseCStringResult.pos -eq $settingsDefinitions.Length) {
                    continue;
                }
                $inlineComment = $parseCStringResult.str;
                $pos = $parseCStringResult.pos;
                if ($settingsDefinitions.Chars($pos) -ne 41) {  # )
                    continue;
                }
            } else {
                continue;
            }

            $newIni.Add("");
            foreach ($line in ($desc -split "`n")) {
                $newIni.Add($line);
            }
            if ($inlineComment.Length -gt 0) {
                $newIni.Add($name + " = " + $value + " " + $inlineComment);
            } else {
                $newIni.Add($name + " = " + $value);
            }
        }
    }


    if (Test-Path "ggxrd_hitbox_overlay.ini") {
        Remove-Item "ggxrd_hitbox_overlay.ini"
    }
    # StreamWriter ignores the cd and $PWD and either uses the cd of the parent script that ran this script or somehow just uses an obsolete cd
    # so I have to write out $PWD explicitly
    # this is a really interesting problem and I wonder how deep it goes. Does it affect launching native executables?
    $writer = New-Object System.IO.StreamWriter("$PWD\ggxrd_hitbox_overlay.ini")  # Default: UTF8NoBOM, "`r`n" newlines
    $writer.NewLine = "`n";  # even on Windows, by default, the INI file is created using \n line breaks, not \r\n
    foreach ($line in $newIni) {
        $writer.WriteLine($line);
    }
    $writer.Close();


    $writer = New-Object System.IO.StreamWriter("$PWD\New_README.md")  # Default: UTF8NoBOM, "`r`n" newlines

    $insideIni = $false;
    Get-Content "README.md" | % {
        if (-not $insideIni) {
            $writer.WriteLine($_);
            if ($_ -eq '```ini') {
                $insideIni = $true;
                foreach ($line in $newIni) {
                    $writer.WriteLine($line);
                }
            }
        } elseif ($_ -eq '```') {
            $writer.WriteLine($_);
            $insideIni = $false;
        }
    }

    $writer.Close();
    if (-not (Test-Path "New_README.md")) {
        Write-Error "Failed to create New_README.md"
        return;
    }

    Remove-Item "README.md"
    Rename-Item -Path "New_README.md" "README.md"

    $currentDate.ToString($dtFormat) | Out-File -FilePath "regenerate_ini_and_update_readme__last_date.txt"

}

task
