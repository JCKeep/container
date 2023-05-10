#![allow(
    non_camel_case_types,
    non_snake_case,
    non_upper_case_globals,
    unused,
    warnings
)]
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[macro_export]
macro_rules! c_str {
    ($str:expr) => {{
        const S: &str = concat!($str, "\0");
        const C: &std::ffi::CStr = match std::ffi::CStr::from_bytes_with_nul(S.as_bytes()) {
            Ok(v) => v,
            Err(_) => panic!("string contains interior NUL"),
        };
        C
    }};
}

#[macro_export]
macro_rules! r_str {
    ($str:expr) => {{
        const C: &'static std::ffi::CStr = match std::ffi::CStr::from_bytes_with_nul($str) {
            Ok(v) => v,
            Err(_) => panic!("string contains interior NUL"),
        };
        const S: &'static str = match C.to_str() {
            Ok(v) => v,
            Err(_) => panic!("string contains interior NUL"),
        };
        S
    }};
}
