
pub fn string_matches(string_str: &str, glob_str: &str) -> bool
{
    // Empty glob
    if glob_str.is_empty() {
        return string_str.is_empty();
    }

    let string = string_str.chars().collect::<Vec<_>>();
    let glob = glob_str.chars().collect::<Vec<_>>();

    let mut string_index = 0;
    let mut glob_index = 0;
    let mut star_pattern_offset = 0;
    loop
    {
        // Test for ending conditions
        if string_index >= string.len() && glob_index + star_pattern_offset + 1 >= glob.len() {
            return true;
        }
        if string_index >= string.len() {
            return false;
        }
        if glob_index >= glob.len() {
            return false;
        }

        let glob_char = glob[glob_index];
        let string_char = string[string_index];
        match glob_char
        {
            '*' =>
            {
                // Star at the end of a glob, will always match
                if glob_index + 1 >= glob.len() {
                    return true;
                }

                // Increment pattern index, wrapping around when the glob finishes
                star_pattern_offset += 1;
                if glob_index + star_pattern_offset >= glob.len() {
                    star_pattern_offset = 1;
                }

                // Match pattern
                let pattern_char = glob[glob_index + star_pattern_offset];
                if pattern_char == '*'
                {
                    glob_index += star_pattern_offset;
                    star_pattern_offset = 0;
                }
                else
                {
                    if pattern_char != '?' && string_char != pattern_char {
                        star_pattern_offset = 0;
                    }
                }

                // Increment string index
                string_index += 1;
            },

            '?' =>
            {
                glob_index += 1;
                string_index += 1;
            },

            _ =>
            {
                if string_char != glob_char {
                    return false;
                }

                glob_index += 1;
                string_index += 1;
            },
        }
    }
}

#[test]
fn globs()
{
    assert_eq!(string_matches("testing", "test*"), true);
    assert_eq!(string_matches("test", "test*"), true);
    assert_eq!(string_matches("tes", "test*"), false);

    assert_eq!(string_matches("testing", "*ing"), true);
    assert_eq!(string_matches("rowing", "*ing"), true);
    assert_eq!(string_matches("noting right", "*ing"), false);
    assert_eq!(string_matches("testinging", "*ing"), true);

    assert_eq!(string_matches("testinging", "test*ing"), true);
    assert_eq!(string_matches("test a thing", "test*ing"), true);
    assert_eq!(string_matches("test a thing not", "test*ing"), false);

    assert_eq!(string_matches("test a complex thing", "test*lex*ing"), true);
    assert_eq!(string_matches("wildcard", "wi?d?ard"), true);
    assert_eq!(string_matches("wirdbard", "wi?d?ard"), true);
}
