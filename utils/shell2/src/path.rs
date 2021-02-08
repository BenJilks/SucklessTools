use std::path::PathBuf;

pub trait ShellPath
{
    fn resolve(&self) -> Self;
    fn get_dir(&self) -> Self;
}

impl ShellPath for PathBuf
{

    fn resolve(&self) -> Self
    {
        if !self.starts_with("~") {
            return self.clone();
        }

        let path = self.strip_prefix("~").unwrap();
        std::env::home_dir().unwrap().join(path)
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
