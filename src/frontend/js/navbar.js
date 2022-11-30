console.clear();

const nav = document.getElementById('side-nav');
const showNavBtn = document.getElementById('show-nav');
const container = document.getElementById('container');
const navWidth = 15; // rems
const navGutter = 1;

nav.addEventListener('click', (event) => {
	if(event.target.classList.contains('sub-menu-link')){
    event.target.classList.toggle('active');
  	const subMenu = event.target.nextElementSibling;
    subMenu.classList.toggle('active');
  }
});

showNavBtn.addEventListener('click', (event) => {
  if (nav.style.left !== '0px') {
    showNavBtn.classList.toggle('open');
    nav.classList.toggle('open');
  	container.classList.toggle('nav-open');
    document.body.style.overflow = 'hidden';
  } else {
    showNavBtn.classList.toggle('open');
    nav.classList.toggle('open');
  	container.classList.toggle('nav-open');
    document.body.style.overflow = 'auto';
  }
}, nav);