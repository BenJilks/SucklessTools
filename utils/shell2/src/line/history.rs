
pub enum HistoryDirection
{
    Back,
    Forward,
}

pub struct History
{
    history: Vec<String>,
    history_index: i32,
}

impl History
{

    pub fn new() -> Self
    {
        Self
        {
            history: Vec::new(),
            history_index: 0,
        }
    }

    pub fn get_history(&mut self, direction: HistoryDirection) -> Option<String>
    {
        match direction
        {
            HistoryDirection::Back =>
            {
                if self.history_index <= 0 {
                    return None;
                }

                self.history_index -= 1;
                Some(self.history[(self.history_index) as usize].clone())
            },

            HistoryDirection::Forward =>
            {
                if self.history_index >= self.history.len() as i32 - 1 {
                    return None;
                }

                self.history_index += 1;
                Some(self.history[self.history_index as usize].clone())
            }
        }
    }

    pub fn push(&mut self, command: String)
    {
        self.history.push(command);
        self.history_index = self.history.len() as i32;
    }

}
