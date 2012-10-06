#define _QF(q,f) __##q##__##f

#define _QDeclaration(q,t,s)\
register t *_QF(q,i),*_QF(q,o); register int _QF(q,n); static t _QF(q,b)[s]; meeting _QF(q,im):2,_QF(q,om):2

#define _QSize(q)\
(sizeof(_QF(q,b))/sizeof(*_QF(q,b)))

#define _QNElem(q)\
(_QF(q,n))

#define _QIPtr(q)\
(_QF(q,i))

#define _QOPtr(q)\
(_QF(q,o))

#define _QBPtr(q)\
(_QF(q,b))

#define _QInit(q)\
_QF(q,n)=0,_QF(q,i)=_QF(q,o)=_QF(q,b)

#define _QPost(q,v)\
join( _QF(q,im) ); *_QF(q,i)=(v) , _QF(q,i)=(_QF(q,i)<&_QF(q,b)[(_QSize(q)-1)])?(_QF(q,i)+1):(_QF(q,b))

#define _QPend(q,r)\
join( _QF(q,om) ); (r)=*_QF(q,o) , _QF(q,o)=(_QF(q,o)<&_QF(q,b)[(_QSize(q)-1)])?(_QF(q,o)+1):(_QF(q,b))

#define _QUse(q)\
{\
_QF(q,uflow):	join(_QF(q,im)); _QF(q,n)++;\
_QF(q,t_oflow):	if(_QF(q,n)>=_QSize(q)) { join(_QF(q,om)); _QF(q,n)--; } else goto _QF(q,io);\
_QF(q,t_uflow):	if(_QF(q,n)<=0) goto _QF(q,uflow);\
_QF(q,io):\
	for(;;)\
		when(_QF(q,im)) {\
			_QF(q,n)++;\
			goto _QF(q,t_oflow);\
		}\
		else\
			when(_QF(q,om)) {\
				_QF(q,n)--;\
				goto _QF(q,t_uflow);\
			}\
}
