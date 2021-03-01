#![allow(unconditional_panic)]
#![allow(const_err)]

use std::io;

fn main() {
    let mut extra_newline = false;

    loop {
        if extra_newline {
            println!();
        }

        extra_newline = true;
        println!("Do what?");
        println!("0 - Quit");
        println!("1 - Call a function");
        println!("2 - Crash");
        println!("3 - Get some Fibonacci numbers");
        println!("4 - Get too many Fibonacci numbers");

        let mut line = String::new();
        io::stdin()
            .read_line(&mut line)
            .expect("How did you even do this?!");

        let raw_len = line.len();
        line = line.trim().to_string();
        println!("You chose {} raw length is {}", line.trim(), raw_len);

        let line: u8 = match line.parse() {
            Ok(value) => value,
            Err(_) => {
                println!("That's not a number or is too big, dingus");
                continue;
            }
        };

        match line {
            0 => break,
            1 => println!("Function returned: {}", make_bigger(8)),
            2 => println!("{}", 5 / 0),
            3 => calc_fib(50),
            4 => calc_fib(255),
            _ => println!("Bogus choice"),
        }

        println!("")
    }
}

fn make_bigger(num: u8) -> u16 {
    (num * 2) as u16
}

fn calc_fib(mut count: u8) {
    let mut x: u128 = 0;
    let mut y: u128 = 1;

    print!("The first {} numbers are {} ", count, x);
    count = count - 1;

    if count > 0 {
        print!("{} ", y);
        count = count - 1;
    }

    while count > 0 {
        let z = x + y;
        print!("{} ", z);
        count = count - 1;
        x = y;
        y = z;
    }

    println!("");
}
