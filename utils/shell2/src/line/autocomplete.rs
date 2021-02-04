use std::path::{Path, PathBuf};
use std::str::FromStr;

pub enum Completion
{
    Single(String),
    Multiple(Vec<String>),
}

fn make_completion(dir: &Path, word: String, is_end: bool) -> Completion
{
    let full = dir.join(word);
    let full_name = full.to_str().unwrap().to_owned();
    if is_end && full.is_file() {
        Completion::Single(full_name + " ")
    } else if is_end && full.is_dir() {
        Completion::Single(full_name + "/")
    } else {
        Completion::Single(full_name)
    }
}

fn from_dir(dir: &Path, is_current_dir: bool, start: &str) -> Option<Completion>
{
    let read_dir = 
        if is_current_dir { 
            std::env::current_dir().unwrap().read_dir() 
        } else { 
            dir.read_dir() 
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
        return Some(make_completion(dir, matches[0].clone(), true));
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
        return Some(make_completion(dir, max_common, false));
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
    let dir = 
        if (path.is_file() || !path.exists()) && path.parent().is_some() {
            path.parent().unwrap().to_path_buf()
        } else {
            path.clone()
        };

    // Verifiy it's a directory
    let is_current_dir = !dir.exists();
    if !is_current_dir && !dir.is_dir() {
        return None;
    }

    // Fetch the options
    let mut start = "";
    if path.is_file() || !path.exists()
    {
        let start_os_str = path.file_name().unwrap_or(std::ffi::OsStr::new(""));
        start = start_os_str.to_str().unwrap();
    }
    from_dir(&dir, is_current_dir, &start)
}
