use crate::path::ShellPath;
use std::path::{Path, PathBuf};
use std::str::FromStr;

pub enum Completion
{
    Single(String),
    Multiple(Vec<String>),
}

fn make_completion(dir: &Path, word: String) -> Completion
{
    let full = dir.join(word);
    let full_name = full.to_str().unwrap().to_owned();
    Completion::Single(full_name)
}

fn make_full_completion(original_dir: &Path, dir: &Path, word: String) -> Completion
{
    let full = dir.join(&word);
    let full_name = original_dir
        .join(&word)
        .to_str().unwrap()
        .to_owned();
    
    if full.is_file() {
        Completion::Single(full_name + " ")
    } else if full.is_dir() {
        Completion::Single(full_name + "/")
    } else {
        Completion::Single(full_name)
    }
}

fn from_dir(original_dir: &Path, dir: &Path, start: &str) -> Option<Completion>
{
    let read_dir = 
        if dir.exists() {
            dir.read_dir()
        } else {
            std::env::current_dir().unwrap().read_dir()
        };
    if read_dir.is_err() {
        return None;
    }

    let mut matches = Vec::<String>::new();
    for entry in read_dir.unwrap()
    {
        let path = entry.unwrap().path();
        let file_name_or_none = path.file_name();
        if file_name_or_none.is_none() {
            continue;
        }
        
        let file_name = file_name_or_none.unwrap().to_str().unwrap();
        if file_name.starts_with(start) {
            matches.push(file_name.to_owned());
        }
    }

    if matches.len() == 1 {
        return Some(make_full_completion(original_dir, dir, matches[0].clone()));
    }

    if matches.len() > 1 
    {
        let mut max_common = matches.first().unwrap().clone();
        for match_ in &matches 
        {
            let len = std::cmp::min(max_common.len(), match_.len());
            for i in 0..len
            {
                if max_common.as_bytes()[i] != match_.as_bytes()[i] 
                {
                    max_common = max_common[0..i].to_owned();
                    break;
                }
            }
        }

        if max_common == start {
            return Some(Completion::Multiple(matches));
        }
        return Some(make_completion(original_dir, max_common));
    }

    return None;
}

pub fn complete(partial: &str) -> Option<Completion>
{
    let path_or_error = PathBuf::from_str(partial);
    if path_or_error.is_err() {
        return None;
    }
    
    // Directory this file is in
    let path = path_or_error.unwrap();
    let dir = path.get_dir();
    let resolved_path = path.resolve();
    let resolved_dir = resolved_path.get_dir();

    // Fetch the options
    let mut start = "";
    if resolved_path.is_file() || !resolved_path.exists()
    {
        let start_os_str = resolved_path.file_name().unwrap_or(std::ffi::OsStr::new(""));
        start = start_os_str.to_str().unwrap();
    }
    from_dir(&dir, &resolved_dir, &start)
}
