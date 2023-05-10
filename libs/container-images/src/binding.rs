#![allow(
    non_camel_case_types,
    non_snake_case,
    non_upper_case_globals,
    unused,
    warnings
)]
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

/// Creates a new [`CStr`] from a string literal.
///
/// The string literal should not contain any `NUL` bytes.
///
/// # Examples
///
/// ```
/// # use kernel::c_str;
/// # use kernel::str::CStr;
/// const MY_CSTR: &CStr = c_str!("My awesome CStr!");
/// ```
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

/// Creates a new [`&str`] from a const c string binding with bindgen.
///
/// # Examples
///
/// ```
/// # use kernel::c_str;
/// # use kernel::str::CStr;
/// // binding::DATA type is [u8; n]
/// const MY_CSTR: &str = r_str!(binding::DATA);
/// ```
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
