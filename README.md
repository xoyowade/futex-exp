An experiment to test wakeup order in mutex/futex. Detailed explanation could be found on my Chinese blog [post](http://xoyo.space/2017/11/mutex-lock-wakeup-order/).

In order to set thread priority, `wait-wake` should be run with sudo.
```
make && sudo ./wait-wake
``` 
