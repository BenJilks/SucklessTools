use std::path::PathBuf;
use nix::unistd::{User, geteuid};

pub trait ShellPath
{
    fn resolve(&self) -> Self;
    fn get_dir(&self) -> Self;
}

impl ShellPath for PathBuf
{

    fn resolve(&self) -> Self
    {
        if self.components().count() == 0 {
            return std::env::current_dir().unwrap();
        }

        if !self.starts_with("~") {
            return self.clone();
        }

        let path = self.strip_prefix("~").unwrap();
        let home = User::from_uid(geteuid()).unwrap().unwrap().dir;
        home.join(path)
    }

    fn get_dir(&self) -> Self
    {
        if self.is_dir() {
            return self.clone();
        }
        
        if self.file_name().unwrap_or_default() == "~" {
            return self.clone();
        }

        let parent = self.parent();
        if parent.is_none() {
            return self.clone();
        }

        return parent.unwrap().to_path_buf();
    }

}
