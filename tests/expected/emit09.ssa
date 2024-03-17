; ModuleID = 'main'

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  br i1 false, label %lor.end2, label %lor.rhs1

lor.rhs:                                          ; preds = %lor.end2
  br label %lor.end

lor.end:                                          ; preds = %lor.rhs, %lor.end2
  %0 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @.str, i32 0, i32 0), i32 1)
  ret i32 0

lor.rhs1:                                         ; preds = %entry
  br label %lor.end2

lor.end2:                                         ; preds = %lor.rhs1, %entry
  %1 = phi i1 [ true, %entry ], [ false, %lor.rhs1 ]
  %2 = zext i1 %1 to i32
  %tobool = icmp ne i32 %2, 0
  br i1 %tobool, label %lor.end, label %lor.rhs
}
