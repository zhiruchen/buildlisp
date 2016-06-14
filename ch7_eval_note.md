# 效果
```
lispy--> / 100 (* 5 (+ 6 -3))
>
regex
  operator|char:1:1 '/'
  expr|number|regex:1:3 '100'
  expr|>
    char:1:7 '('
    operator|char:1:8 '*'
    expr|number|regex:1:10 '5'
    expr|>
      char:1:12 '('
      operator|char:1:13 '+'
      expr|number|regex:1:15 '6'
      expr|number|regex:1:17 '-3'
      char:1:19 ')'
    char:1:20 ')'
  regex
6
```
