fn main(){
    let mut y : [f64;3] = [1.0 , 2.0 , 3.0];
    y[1] += y[2];
    println!("The fiver: {}", y[1]);
}
