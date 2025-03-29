fn main(){
    let x : bool = false;
    let mut y : i64 = 3;
    if(x && ((y / 0) > 1)) {
        y = 20;
    } else {
        y = 1111111;
    }
    println!("Bunch of ones {}", y);
}