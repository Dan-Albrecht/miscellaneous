use crate::{assemblyStuff::halt::haltLoop, cursorStuff::writeStringOnNewline};

// Root System Description Pointer
#[repr(C, align(16))]
struct RSDP {
    RSDP_I: RSDP_I,
}

#[repr(C, packed)]
struct RSDP_I {
    Signature: [u8; 8],
}

pub fn readBytes() {
    // We're going to assume this won't appear in the Extended BIOS Data Area (EBDA)
    // and just search the BIOS readonly area.
    let mut address: usize = 0x0E0000;
    loop {
        let ptr = address as *const RSDP;
        if checkSignature(ptr) {
            unsafe { writeStringOnNewline(b"Found it!") };
            haltLoop();
        }

        address = address + 16;
        if address > 0x0FFFFF {
            unsafe { writeStringOnNewline(b"Didn't find RSDP. Halting.") };
            haltLoop();
        }
    }
}

fn checkSignature(ptr: *const RSDP) -> bool {
    let expected = *b"RSD PTR ";
    unsafe {
        let toCheck = (*ptr).RSDP_I.Signature;
        if toCheck == expected {
            return  true;
        }
    }

    return false;
}
