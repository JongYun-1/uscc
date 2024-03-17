; ModuleID = 'main'

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  br i1 false, label %and.rhs, label %and.end

lor.rhs:                                          ; preds = %and.end
  br label %lor.end

lor.end:                                          ; preds = %lor.rhs, %and.end
  br i1 true, label %if.then, label %if.else

and.rhs:                                          ; preds = %entry
  br label %and.end

and.end:                                          ; preds = %and.rhs, %entry
  %0 = phi i1 [ false, %entry ], [ true, %and.rhs ]
  %1 = zext i1 %0 to i32
  %tobool = icmp ne i32 %1, 0
  br i1 %tobool, label %lor.end, label %lor.rhs

if.then:                                          ; preds = %lor.end
  br label %if.end

if.end:                                           ; preds = %if.then, %if.else
  %Phi = phi i32 [ 20, %if.then ], [ 10, %if.else ]
  %2 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @.str, i32 0, i32 0), i32 %Phi)
  ret i32 0

if.else:                                          ; preds = %lor.end
  br label %if.end
}
