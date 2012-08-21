#define _QF(q,f) __##q##__##f

#define _QDeclaration(q,t,s)\
register t *_QF(q,i),*_QF(q,o); register int _QF(q,n); static t _QF(q,b)[s]

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
while(\
	_QF(q,n)>=_QSize(q) ? \
	1\
	:\
	(_QF(q,n)++, *_QF(q,i)=(v), _QF(q,i)=(_QF(q,i)<&_QF(q,b)[(_QSize(q)-1)]) ? _QF(q,i)+1 : _QF(q,b), 0)\
)

#define _QPend(q,r)\
while(\
	_QF(q,n)==0 ? \
	1\
	:\
	(_QF(q,n)--, (r)=*_QF(q,o), _QF(q,o)=(_QF(q,o)<&_QF(q,b)[(_QSize(q)-1)]) ? _QF(q,o)+1 : _QF(q,b), 0)\
)

#define _QUse(q) { }
