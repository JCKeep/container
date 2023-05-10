#![feature(const_cstr_methods)]
use libc::c_void;
use std::ffi::CStr;

mod binding;
mod image_pack;

#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}

#[no_mangle]
pub extern "C" fn rust_image_confirm(name: *const c_void) -> i32 {
    let s = unsafe { CStr::from_ptr(name.cast()).to_str().unwrap() };

    if let Err(e) = image_pack::pack_image(s) {
        eprintln!("image_pack::pack_image {e:?}");
        return -1;
    }

    if let Err(e) = image_pack::gz_compress_image(s) {
        eprintln!("image_pack::gz_compress_image {e:?}");
        return -1;
    }

    0
}

#[cfg(test)]
mod tests {
    use std::ffi::CString;

    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }

    #[test]
    fn image_test() {
        let s = CString::new("nginx").unwrap();
        if rust_image_confirm(s.as_ptr().cast()) < 0 {
            panic!()
        }
    }
}
