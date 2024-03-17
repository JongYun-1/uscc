; ModuleID = 'main'

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  br label %while.cond

while.cond:                                       ; preds = %if.end, %entry
  %Phi = phi i32 [ %dec, %if.end ], [ 10, %entry ]
  %Phi4 = phi i32 [ %Phi5, %if.end ], [ 0, %entry ]
  %gt = icmp sgt i32 %Phi, 0
  br i1 %gt, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %dec = sub i32 %Phi, 1
  %0 = icmp ne i32 0, 0
  br i1 %0, label %if.then, label %if.else

while.end:                                        ; preds = %while.cond
  %1 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @.str, i32 0, i32 0), i32 %Phi4)
  ret i32 0

if.then:                                          ; preds = %while.body
  br label %if.end

if.end:                                           ; preds = %if.then, %if.else
  %Phi5 = phi i32 [ 100, %if.then ], [ 50, %if.else ]
  br label %while.cond

if.else:                                          ; preds = %while.body
  br label %if.end
}
