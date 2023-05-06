#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
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
