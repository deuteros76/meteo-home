#include <string>


const std::string CSS_CODE="<style> \
.c, \
body { \
 text-align: center; \
 font-family: 'Roboto', sans-serif; \
 background-color: #424242; \
 color: #b1b0b0; \
 background-repeat: no-repeat; \
 background-size: cover; \
 background-position: center; \
} \
div,\
input,\
select {\
 padding:5px;\
 font-size:1em;\
 margin:5px 0;\
 box-sizing:border-box\
}\
input,\
button,\
select,\
.msg {\
 border-radius:.3rem;\
 width: 100%;\
 background-color: #b1b0b0; \
 color: #424242; \
}\
input[type=radio] {\
 width:auto;\
}\
input[type=checkbox] {\
 width: auto;\
 background-color: #b1b0b0;\
 color: #47b147;\
}\
input[type=checkbox]:checked {\
  accent-color: #47b147;\
}\
input[type=range] {\
  accent-color: #47b147;\
}\
button,\
input[type='button'],\
input[type='submit'] {\
 cursor:pointer;\
 border:0;\
 background-color:#47b147;\
 color:#fff;\
 line-height:2.4rem;\
 font-size:1.2rem;\
 width:100%\
}\
input[type='file'] {\
 border:1px solid #47b147\
}\
.wrap {\
 text-align:left;\
 display:inline-block;\
 min-width:260px;\
 max-width:500px;\
}\
a {\
 color:#000;\
 font-weight:700;\
 text-decoration:none\
}\
a:hover {\
 color:#47b147;\
 text-decoration:underline\
}\
.q {\
 height:16px;\
 margin:0;\
 padding:0 5px;\
 text-align:right;\
 min-width:38px;\
 float:right\
}\
.q.q-0:after {\
 background-position-x:0\
}\
.q.q-1:after {\
 background-position-x:-16px\
}\
.q.q-2:after {\
 background-position-x:-32px\
}\
.q.q-3:after {\
 background-position-x:-48px\
}\
.q.q-4:after {\
 background-position-x:-64px\
}\
.q.l:before {\
 background-position-x:-80px;\
 padding-right:5px\
}\
.ql .q {\
 float:left\
}\
.q:after,\
.q:before {\
 content:'';\
 width:16px;\
 height:16px;\
 display:inline-block;\
 background-repeat:no-repeat;\
 background-position: 16px 0;\
 background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');\
}\
@media (-webkit-min-device-pixel-ratio: 2),(min-resolution: 192dpi) {\
 .q:before,\
 .q:after {\
  background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALwAAAAgCAMAAACfM+KhAAAALVBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAOrOgAAAADnRSTlMAESIzRGZ3iJmqu8zd7gKjCLQAAACmSURBVHgB7dDBCoMwEEXRmKlVY3L//3NLhyzqIqSUggy8uxnhCR5Mo8xLt+14aZ7wwgsvvPA/ofv9+44334UXXngvb6XsFhO/VoC2RsSv9J7x8BnYLW+AjT56ud/uePMdb7IP8Bsc/e7h8Cfk912ghsNXWPpDC4hvN+D1560A1QPORyh84VKLjjdvfPFm++i9EWq0348XXnjhhT+4dIbCW+WjZim9AKk4UZMnnCEuAAAAAElFTkSuQmCC');\
  background-size: 95px 16px;\
 }\
}\
.msg {\
 padding:20px;\
 margin:20px 0;\
 border:1px solid #eee;\
 border-left-width:5px;\
 border-left-color:#777\
}\
.msg h4 {\
 margin-top:0;\
 margin-bottom:5px\
}\
.msg.P {\
 border-left-color:#47b147\
}\
.msg.P h4 {\
 color:#47b147\
}\
.msg.D {\
 border-left-color:#dc3630\
}\
.msg.D h4 {\
 color:#dc3630\
}\
.msg.S {\
 border-left-color: #5cb85c\
}\
.msg.S h4 {\
 color: #5cb85c\
}\
dt {\
 font-weight:bold\
}\
dd {\
 margin:0;\
 padding:0 0 0.5em 0;\
 min-height:12px\
}\
td {\
 vertical-align: top;\
}\
.h {\
 display:none\
}\
button {\
 transition: 0s opacity;\
 transition-delay: 3s;\
 transition-duration: 0s;\
 cursor: pointer\
}\
button.D {\
 background-color:#dc3630\
}\
button:active {\
 opacity:50% !important;\
 cursor:wait;\
 transition-delay: 0s\
}\
body.invert,\
body.invert a,\
body.invert h1 {\
 background-color:#060606;\
 color:#fff;\
}\
body.invert .msg {\
 color:#fff;\
 background-color:#282828;\
 border-top:1px solid #555;\
 border-right:1px solid #555;\
 border-bottom:1px solid #555;\
}\
body.invert .q[role=img] {\
 -webkit-filter:invert(1);\
 filter:invert(1);\
}\
:disabled {\
 opacity: 0.5;\
}\
.panel { border-radius: 25px; border: 2px solid #47b147; padding: 20px; margin: 20px;} \
h1 {  position: relative;  padding: 0;  margin: 0;  -webkit-transition: all 0.4s ease 0s;  -o-transition: all 0.4s ease 0s; transition: all 0.4s ease 0s;}h1 span { display: block;  font-size: 0.5em;  line-height: 1.3;} .four h1 {  text-align: center;  padding-bottom: 0.7em;  font-size: 24px;} .four h1 span {  font-weight: 300;  word-spacing: 3px;  line-height: 2em;  padding-bottom: 0.35em;  color: rgba(0, 0, 0, 0.5);}.four h1:before {  position: absolute;  left: 0;  bottom: 0;  width: 200px;  height: 1px;  content: "";  left: 50%;  margin-left: -100px;  background-color: #777;} \
</style>";