use std::{env, path::PathBuf};

fn main() {
    println!("cargo:rustc-link-lib=dl");
    println!("cargo:rerun-if-changed=binding.h");

    let bindings = bindgen::Builder::default()
        .header("binding.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("couldn't write bindings!");
}
