use std::ffi::CStr;

use libc::c_void;

mod image_pack;

#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}

#[no_mangle]
pub extern "C" fn rust_image_confirm(name: *const c_void) -> i32 {
    let s = unsafe { CStr::from_ptr(name.cast()).to_str().unwrap() };

    if let Err(e) = image_pack::pack_image(s) {
        eprintln!("image_pack::pack_image {:?}", e);
        return -1;
    }

    if let Err(e) = image_pack::gz_compress_image(s) {
        eprintln!("image_pack::gz_compress_image {:?}", e);
        return -1;
    }

    0
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }

    #[test]
    fn tar_test() {
        use std::fs::File;
        use tar::Builder;

        let file = File::create("tmp.tar").unwrap();
        let mut a = Builder::new(file);

        a.append_path("Cargo.toml").unwrap();
        a.append_dir_all("src", "src").unwrap();
        a.finish().unwrap();
    }
}
