#![allow(dead_code)]
use std::{
    ffi::CString,
    fs::{File, OpenOptions},
    io,
};

use flate2::{write::GzEncoder, Compression};

/// add the image to a tar file
pub fn pack_image(name: &str) -> io::Result<()> {
    let filep = format!("{}{}", name, ".tar");

    use std::process::Command;
    let mut child = Command::new("tar")
        .current_dir("/root/D/kernel/demo-container/images")
        .arg("-cvf")
        .arg(filep)
        .arg(name)
        .spawn()?;
    child.wait()?;

    Ok(())
}

/// use gzip compress the image
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
