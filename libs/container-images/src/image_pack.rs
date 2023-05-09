#![allow(dead_code)]
use std::{
    ffi::CString,
    fs::{File, OpenOptions},
    io,
};

use flate2::{write::GzEncoder, Compression};

pub fn pack_image(name: &str) -> io::Result<()> {
    let path = format!("/root/D/kernel/demo-container/images/{}", name);
    let filep = format!("{}{}", path, ".tar");
    let file = OpenOptions::new()
        .write(true)
        .read(true)
        .truncate(true)
        .create(true)
        .open(filep)?;
    let mut a = tar::Builder::new(file);

    let _ = a.append_dir_all(name, path);
    a.finish()?;

    Ok(())
}

pub fn gz_compress_image(name: &str) -> io::Result<()> {
    let path = format!("/root/D/kernel/demo-container/images/{}", name);
    let file = File::open(format!("{}{}", path, ".tar"))?;

    let image_path = format!("{}{}", path, ".tar.gz");
    let mut image = OpenOptions::new()
        .read(true)
        .write(true)
        .truncate(true)
        .create(true)
        .open(image_path)?;

    let mut encoder = GzEncoder::new(file, Compression::best());

    io::copy(&mut encoder, &mut image)?;
    unsafe {
        let s = CString::new(format!("{}{}", path, ".tar")).unwrap();
        libc::remove(s.as_ptr());
    }

    Ok(())
}
