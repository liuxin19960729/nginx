# note
## Adding new modules
[Adding new modules](https://nginx.org/en/docs/dev/development_guide.html#adding_new_modules)
```sh
# configure 使用一般设置未模块名字 
ngx_addon_name=xx

# ngx_addon_dir  等同于 configure --add-module=PATH 的 PATH
ngx_module_srcs="$ngx_addon_dir/xxx.c"


```
