fn foo(i : i64) -> i64{
    let b : i64 = i + 4;
    return b;
}

fn main(){
    let mut x : i64 = 1;
    x = foo(x);
    println!("The fiver: {}", x);
}