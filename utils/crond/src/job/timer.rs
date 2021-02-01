extern crate json;
extern crate chrono;
use std::result::Result;
use std::ops::{Add, RangeInclusive};
use std::collections::HashMap;
use json::JsonValue;
use chrono::prelude::{Timelike, Datelike};
use chrono::{DateTime, Local};

#[derive(PartialEq, Eq, Hash, Clone, Debug)]
enum Period
{
    Second,
    Minute,
    Hour,
    Weekday,
    Day,
    Month,
}

const PERIOD_ORDER: &[Period] = 
&[
    Period::Month,
    Period::Day,
    Period::Weekday,
    Period::Hour,
    Period::Minute,
    Period::Second,
];

impl Period
{

    pub fn from_str(name: &str) -> Option<Self>
    {
        // TODO: Do this with macros maybe?
        match name
        {
            "Second" => Some(Self::Second),
            "Minute" => Some(Self::Minute),
            "Hour" => Some(Self::Hour),
            "Weekday" => Some(Self::Weekday),
            "Day" => Some(Self::Day),
            "Month" => Some(Self::Month),
            _ => None,
        }
    }

    pub fn valid_range(&self) -> RangeInclusive<u32>
    {
        match self
        {
            Self::Second => 1..=60,
            Self::Minute => 1..=60,
            Self::Hour => 1..=24,
            Self::Weekday => 1..=7,
            Self::Day => 1..=31,
            Self::Month => 1..=12,
        }
    }

}

pub struct Time
{
    period: Period,
    value: u32,
}

impl Time
{

    pub fn load(key: &str, value: &JsonValue) -> Result<Self, String>
    {
        let period_or_error = Period::from_str(key);
        if period_or_error.is_none() {
            return Err(format!("Invalid period '{}'", key));
        }

        let period = period_or_error.unwrap();
        let valid_range = period.valid_range();
        if !value.is_number() || !valid_range.contains(&value.as_u32().unwrap()) {
            return Err(format!("Invalid value '{}'", value));
        }

        return Ok(Time
        {
            period: period,
            value: value.as_u32().unwrap(),
        });
    }

    pub fn compair_now(&self) -> i32
    {
        let now = Local::now();
        match self.period
        {
            Period::Second => now.second() as i32 - self.value as i32,
            Period::Minute => now.minute() as i32 - self.value as i32,
            Period::Hour => now.hour() as i32 - self.value as i32,
            Period::Weekday => (now.weekday().num_days_from_monday() + 1) as i32 - self.value as i32,
            Period::Day => now.day() as i32 - self.value as i32,
            Period::Month => now.month() as i32 - self.value as i32,
        }
    }

    pub fn valid_until(&self) -> DateTime<Local>
    {
        let now = Local::now();
        match self.period
        {
            Period::Second => 
                now.add(chrono::Duration::minutes(1))
                .with_second(0).unwrap(),

            Period::Minute => 
                now.add(chrono::Duration::hours(1))
                .with_minute(0).unwrap(),

            Period::Hour | Period::Weekday => 
                now.add(chrono::Duration::days(1))
                .with_minute(0).unwrap()
                .with_hour(0).unwrap(),

            Period::Day => 
                if now.month() >= 12 {
                    now.with_year(now.year() + 1).unwrap()
                    .with_day(1).unwrap()
                    .with_month(1).unwrap()
                } else {
                    now.with_day(1).unwrap()
                    .with_month(now.month() + 1).unwrap()
                }
                .with_minute(0).unwrap()
                .with_hour(0).unwrap(),

            Period::Month => 
                now.add(chrono::Duration::days(365))
                .with_minute(0).unwrap()
                .with_hour(0).unwrap()
                .with_ordinal(1).unwrap(),
        }
    }

}

pub struct Timer
{
    times: HashMap<Period, Time>,
}

impl Timer
{

    pub fn load(object: &JsonValue) -> Result<Self, String>
    {
        if !object.is_object() {
            return Err("No timer".to_owned());
        }

        let time_object = &object["time"];
        if !time_object.is_object() {
            return Err("No time".to_owned());
        }

        let mut times = HashMap::<Period, Time>::new();
        for (key, value) in time_object.entries()
        {
            let time_or_error = Time::load(key, value);
            if time_or_error.is_err() {
                return Err(time_or_error.err().unwrap());
            }

            let time = time_or_error.ok().unwrap();
            times.insert(time.period.clone(), time);
        }

        return Ok(Self
        {
            times: times,
        })
    }

    pub fn is_condition_met(&self) -> bool
    {
        for period in PERIOD_ORDER
        {
            let time = self.times.get(&period);
            if time.is_none() {
                continue;
            }

            let diff = time.unwrap().compair_now();
            if diff > 0 {
                return true;
            } else if diff < 0 {
                return false;
            } else {
                continue;
            }
        }

        return true;
    }

    pub fn valid_until(&self) -> DateTime<Local>
    {
        let mut max_date = Local::now();
        for (_, time) in &self.times {
            max_date = max_date.max(time.valid_until());
        }

        return max_date;
    }

}
